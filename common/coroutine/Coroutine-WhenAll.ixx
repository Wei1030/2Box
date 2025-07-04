// ReSharper disable CppMemberFunctionMayBeStatic
export module Coroutine:WhenAll;

import std;
import :Concept;
import :PromiseBase;
import :Cancellation;
import :LazyTask;

namespace coro
{
	struct WhenAllCounter
	{
		explicit WhenAllCounter(std::size_t count)
			: counter(count + 1) // 由于任务需要先执行, tryWaitAndReturnNextIfNeed后执行,所以tryWaitAndReturnNextIfNeed也要减计数参与同步
		{
		}

		void startFirstWait(std::coroutine_handle<> next)
		{
			cont = next;
			bHasStartFirstWait.store(true, std::memory_order_relaxed);
		}

		std::coroutine_handle<> tryWaitAndReturnNextIfNeed()
		{
			if (counter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				return cont;
			}
			return std::noop_coroutine();
		}

		std::coroutine_handle<> finishAndReturnNextIfNeed() noexcept
		{
			std::size_t oldCount = counter.fetch_sub(1, std::memory_order_acq_rel);
			if (oldCount == 2)
			{
				// 任务全部结束,且外部还在首次等待(全部任务无异常)
				bool expectedHasStartFirstWait = true;
				if (bHasStartFirstWait.compare_exchange_strong(expectedHasStartFirstWait, false, std::memory_order_relaxed))
				{
					return cont;
				}
			}
			else if (oldCount == 1)
			{
				return cont;
			}
			return std::noop_coroutine();
		}

		// 因异常而结束
		std::coroutine_handle<> finishAndReturnNextIfNeed(const std::exception_ptr& e) noexcept
		{
			bool expectedHasStartFirstWait = true;
			if (bHasStartFirstWait.compare_exchange_strong(expectedHasStartFirstWait, false, std::memory_order_relaxed))
			{
				// 首个异常任务
				exception = e;
				counter.fetch_sub(1, std::memory_order_acq_rel);
				return cont;
			}

			// 不是首个异常的任务, 判断任务是否已经全部完成且外部已经在二次等待
			if (counter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				return cont;
			}
			return std::noop_coroutine();
		}

		std::atomic<std::size_t> counter;
		std::atomic<bool> bHasStartFirstWait{false};
		std::exception_ptr exception{};

		std::coroutine_handle<> cont{};
	};

	template <typename T>
	class WhenAllPerTaskPromise : public PromiseTmplBase<T>
	{
	public:
		std::suspend_always initial_suspend() noexcept
		{
			return {};
		}

		std::coroutine_handle<> finish() noexcept
		{
			if (PromiseBase::m_disc == PromiseBase::Discriminator::Exception)
			{
				return m_counter->finishAndReturnNextIfNeed(PromiseTmplBase<T>::except);
			}
			return m_counter->finishAndReturnNextIfNeed();
		}

		auto final_suspend() noexcept
		{
			struct FinalAwaiter
			{
				WhenAllPerTaskPromise& self;

				bool await_ready() const noexcept { return false; }

				std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept
				{
					return self.finish();
				}

				void await_resume() const noexcept
				{
				}
			};
			return FinalAwaiter{*this};
		}

		const std::stop_token& getCancellationToken() const noexcept
		{
			return m_token;
		}

		void setCancellationToken(std::stop_token token) noexcept
		{
			m_token = std::move(token);
		}

		void setCounter(WhenAllCounter& counter)
		{
			m_counter = &counter;
		}

	private:
		std::stop_token m_token;
		WhenAllCounter* m_counter{nullptr};
	};

	template <typename T>
	class WhenAllPerTask
	{
	public:
		struct promise_type final : public WhenAllPerTaskPromise<T>
		{
			WhenAllPerTask get_return_object() noexcept
			{
				return WhenAllPerTask{std::coroutine_handle<promise_type>::from_promise(*this)};
			}
		};

		WhenAllPerTask() = delete;
		WhenAllPerTask(const WhenAllPerTask&) = delete;

		WhenAllPerTask(WhenAllPerTask&& that) noexcept : m_coro(std::exchange(that.m_coro, {}))
		{
		}

		~WhenAllPerTask()
		{
			if (m_coro)
			{
				if (!m_coro.done())
				{
					std::terminate();
				}
				m_coro.destroy();
			}
		}

		WhenAllPerTask& operator=(const WhenAllPerTask&) = delete;
		WhenAllPerTask& operator=(WhenAllPerTask&& that) = delete;

		void start(WhenAllCounter& counter)
		{
			m_coro.promise().setCounter(counter);
			m_coro.resume();
		}

		using ResultTransferT = std::conditional_t<
			std::is_reference_v<T>,
			T, // 保持引用类型不变
			std::add_rvalue_reference_t<T> // 值类型转右值引用
		>;

		ResultTransferT transferResult()
		{
			return static_cast<ResultTransferT>(m_coro.promise().getValue());
		}

	private:
		explicit WhenAllPerTask(std::coroutine_handle<promise_type> coro) noexcept : m_coro(coro)
		{
		}

		std::coroutine_handle<promise_type> m_coro = nullptr;
	};

	template <awaitable T>
	auto when_all_per_task_impl(T a, std::stop_token token)
		-> WhenAllPerTask<typename AwaitableTraits<std::remove_reference_t<T>>::NonVoidAwaitResultT>
	{
		using AwaitResultT = typename AwaitableTraits<std::remove_reference_t<T>>::AwaitResultT;
		using NonVoidAwaitResultT = typename AwaitableTraits<std::remove_reference_t<T>>::NonVoidAwaitResultT;
		using RefResultT = std::add_lvalue_reference_t<NonVoidAwaitResultT>;

		co_await set_cancellation_token(std::move(token));

		if constexpr (std::is_void_v<AwaitResultT>)
		{
			co_await a;
			co_return NonVoidAwaitResultT{};
		}
		else
		{
			RefResultT result = co_await a;
			if constexpr (std::is_reference_v<AwaitResultT>)
			{
				co_return result;
			}
			else
			{
				co_return std::move(result);
			}
		}
	}

	template <typename... Tasks>
	class WhenAllHelper
	{
	public:
		explicit WhenAllHelper(Tasks&&... tasks)
			noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Tasks>...>)
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(tasks)...)
		{
		}

		explicit WhenAllHelper(std::tuple<Tasks...>&& tasks)
			noexcept(std::is_nothrow_move_constructible_v<std::tuple<Tasks...>>)
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(tasks))
		{
		}

		WhenAllHelper(WhenAllHelper&& other) noexcept
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(other.m_tasks))
		{
		}

		WhenAllCounter& getCounter() noexcept
		{
			return m_counter;
		}

		std::tuple<Tasks...>& getTasks() noexcept
		{
			return m_tasks;
		}

		auto firstWaitReadyOrError() noexcept
		{
			struct Awaiter
			{
				WhenAllHelper& self;
				bool await_ready() const noexcept { return false; }

				void await_suspend(std::coroutine_handle<> cont) noexcept
				{
					self.getCounter().startFirstWait(cont);
					self.startTasks(std::make_integer_sequence<std::size_t, sizeof...(Tasks)>{});
				}

				void await_resume() noexcept
				{
				}
			};
			return Awaiter{*this};
		}

		auto thenWaitAllDone() noexcept
		{
			struct Awaiter
			{
				WhenAllHelper& self;
				bool await_ready() const noexcept { return false; }

				std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept
				{
					return self.getCounter().tryWaitAndReturnNextIfNeed();
				}

				void await_resume() noexcept
				{
				}
			};
			return Awaiter{*this};
		}

	private:
		template <std::size_t... Is>
		void startTasks(std::integer_sequence<std::size_t, Is...>) noexcept
		{
			(std::get<Is>(m_tasks).start(m_counter), ...);
		}

	private:
		WhenAllCounter m_counter;
		std::tuple<Tasks...> m_tasks;
	};


	template <std::size_t... Is, awaitable... Ts>
	auto when_all_impl(std::index_sequence<Is...>, Ts... awaitables)
		-> LazyTask<std::tuple<typename AwaitableTraits<std::remove_reference_t<Ts>>::NonVoidAwaitResultT...>>
	{
		std::stop_source cancelSrc;
		std::stop_callback cancelWhenParentCanceled{
			co_await get_current_cancellation_token(),
			[&]() noexcept { cancelSrc.request_stop(); }
		};

		WhenAllHelper whenAllHelper{
			when_all_per_task_impl(std::move(awaitables), cancelSrc.get_token())
			...
		};

		co_await whenAllHelper.firstWaitReadyOrError();

		const WhenAllCounter& counter = whenAllHelper.getCounter();
		if (counter.exception)
		{
			cancelSrc.request_stop();
		}

		co_await whenAllHelper.thenWaitAllDone();

		if (counter.exception)
		{
			std::rethrow_exception(counter.exception);
		}

		auto& tasks = whenAllHelper.getTasks();
		co_return std::tuple<typename AwaitableTraits<std::remove_reference_t<Ts>>::NonVoidAwaitResultT...>{
			std::get<Is>(tasks).transferResult()
			...
		};
	}

	export
	template <awaitable... Ts>
		requires (sizeof...(Ts) > 0)
	auto when_all(Ts... awaitables)
	{
		return when_all_impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(awaitables)...);
	}
}
