// ReSharper disable CppMemberFunctionMayBeStatic
export module Coroutine:WhenAny;

import std;
import :Concept;
import :PromiseBase;
import :Cancellation;
import :LazyTask;

namespace coro
{
	struct WhenAnyCounter
	{
		explicit WhenAnyCounter(std::size_t count)
			: counter(count + 1)
			  , index(0)

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

		std::coroutine_handle<> finishAndReturnNextIfNeed(std::size_t n) noexcept
		{
			bool expectedHasStartFirstWait = true;
			if (bHasStartFirstWait.compare_exchange_strong(expectedHasStartFirstWait, false, std::memory_order_relaxed))
			{
				// 首个完成的任务
				index = n;
				counter.fetch_sub(1, std::memory_order_acq_rel);
				return cont;
			}

			// 不是首个完成的任务, 判断任务是否已经全部完成且外部已经在二次等待
			if (counter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				return cont;
			}
			return std::noop_coroutine();
		}

		// 因异常而结束
		std::coroutine_handle<> finishAndReturnNextIfNeed(std::size_t n, const std::exception_ptr& e) noexcept
		{
			std::size_t oldCount = counter.fetch_sub(1, std::memory_order_acq_rel);
			if (oldCount == 2)
			{
				// 任务全部结束,且外部还在首次等待(全部任务都异常了)
				bool expectedHasStartFirstWait = true;
				if (bHasStartFirstWait.compare_exchange_strong(expectedHasStartFirstWait, false, std::memory_order_relaxed))
				{
					index = n;
					exception = e;
					return cont;
				}
			}
			else if (oldCount == 1)
			{
				return cont;
			}
			return std::noop_coroutine();
		}

		std::atomic<std::size_t> counter;
		std::size_t index;
		std::atomic<bool> bHasStartFirstWait{false};

		std::exception_ptr exception{};

		std::coroutine_handle<> cont{};
	};

	template <typename T>
	class WhenAnyPerTaskPromise : public PromiseTmplBase<T>
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
				return m_counter->finishAndReturnNextIfNeed(m_index, PromiseTmplBase<T>::except);
			}
			return m_counter->finishAndReturnNextIfNeed(m_index);
		}

		auto final_suspend() noexcept
		{
			struct FinalAwaiter
			{
				WhenAnyPerTaskPromise& self;

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

		void setCounterAndIndex(WhenAnyCounter& counter, std::size_t index) noexcept
		{
			m_counter = &counter;
			m_index = index;
		}

	private:
		std::stop_token m_token;
		WhenAnyCounter* m_counter{nullptr};
		std::size_t m_index{0};
	};

	template <typename T>
	class WhenAnyPerTask
	{
	public:
		struct promise_type final : public WhenAnyPerTaskPromise<T>
		{
			WhenAnyPerTask get_return_object() noexcept
			{
				return WhenAnyPerTask{std::coroutine_handle<promise_type>::from_promise(*this)};
			}
		};

		WhenAnyPerTask() = delete;
		WhenAnyPerTask(const WhenAnyPerTask&) = delete;

		WhenAnyPerTask(WhenAnyPerTask&& that) noexcept : m_coro(std::exchange(that.m_coro, {}))
		{
		}

		~WhenAnyPerTask()
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

		WhenAnyPerTask& operator=(const WhenAnyPerTask&) = delete;
		WhenAnyPerTask& operator=(WhenAnyPerTask&& that) = delete;

		void start(WhenAnyCounter& counter, std::size_t index)
		{
			m_coro.promise().setCounterAndIndex(counter, index);
			m_coro.resume();
		}

		std::add_pointer_t<T> transferResult()
			requires (std::is_reference_v<T>)
		{
			return &m_coro.promise().getValue();
		}

		std::add_rvalue_reference_t<T> transferResult()
			requires (!std::is_reference_v<T>)
		{
			return static_cast<std::add_rvalue_reference_t<T>>(m_coro.promise().getValue());
		}

	private:
		explicit WhenAnyPerTask(std::coroutine_handle<promise_type> coro) noexcept : m_coro(coro)
		{
		}

		std::coroutine_handle<promise_type> m_coro = nullptr;
	};

	template <awaitable T>
	auto when_any_per_task_impl(T a, std::stop_token token)
		-> WhenAnyPerTask<typename AwaitableTraits<std::remove_reference_t<T>>::NonVoidAwaitResultT>
	{
		using AwaitResultT = typename AwaitableTraits<std::remove_reference_t<T>>::AwaitResultT;
		using NonVoidAwaitResultT = typename AwaitableTraits<std::remove_reference_t<T>>::NonVoidAwaitResultT;

		co_await set_cancellation_token(std::move(token));

		if constexpr (std::is_void_v<AwaitResultT>)
		{
			co_await a;
			co_return NonVoidAwaitResultT{};
		}
		else
		{
			std::add_lvalue_reference_t<NonVoidAwaitResultT> result = co_await a;
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
	class WhenAnyHelper
	{
	public:
		explicit WhenAnyHelper(Tasks&&... tasks)
			noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Tasks>...>)
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(tasks)...)
		{
		}

		explicit WhenAnyHelper(std::tuple<Tasks...>&& tasks)
			noexcept(std::is_nothrow_move_constructible_v<std::tuple<Tasks...>>)
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(tasks))
		{
		}

		WhenAnyHelper(WhenAnyHelper&& other) noexcept
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(other.m_tasks))
		{
		}

		WhenAnyCounter& getCounter() noexcept
		{
			return m_counter;
		}

		std::tuple<Tasks...>& getTasks() noexcept
		{
			return m_tasks;
		}

		auto waitAnyDone() noexcept
		{
			struct Awaiter
			{
				WhenAnyHelper& self;
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

		auto waitAllDone() noexcept
		{
			struct Awaiter
			{
				WhenAnyHelper& self;
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
			(std::get<Is>(m_tasks).start(m_counter, Is), ...);
		}

	private:
		WhenAnyCounter m_counter;
		std::tuple<Tasks...> m_tasks;
	};

	template <std::size_t I, awaitable... Ts>
	auto make_when_any_result_helper(std::tuple<WhenAnyPerTask<typename AwaitableTraits<std::remove_reference_t<Ts>>::NonVoidAwaitResultT>...>& tasks)
		-> std::variant<typename AwaitableTraits<std::remove_reference_t<Ts>>::NonVoidRefResultT...>
	{
		return std::variant<typename AwaitableTraits<std::remove_reference_t<Ts>>::NonVoidRefResultT...>{std::in_place_index<I>, std::get<I>(tasks).transferResult()};
	}

	template <std::size_t... Is, awaitable... Ts>
	auto when_any_impl(std::index_sequence<Is...>, Ts... awaitables)
		-> LazyTask<std::variant<typename AwaitableTraits<std::remove_reference_t<Ts>>::NonVoidRefResultT...>>
	{
		std::stop_source cancelSrc;
		std::stop_callback cancelWhenParentCanceled{
			co_await get_current_cancellation_token(),
			[&]() noexcept { cancelSrc.request_stop(); }
		};

		WhenAnyHelper whenAnyHelper{
			when_any_per_task_impl(std::move(awaitables), cancelSrc.get_token())
			...
		};

		co_await whenAnyHelper.waitAnyDone();

		cancelSrc.request_stop();

		co_await whenAnyHelper.waitAllDone();

		const WhenAnyCounter& counter = whenAnyHelper.getCounter();
		if (counter.exception)
		{
			std::rethrow_exception(counter.exception);
		}

		auto& tasks = whenAnyHelper.getTasks();
		
		constexpr std::array factories{
			&make_when_any_result_helper<Is, Ts...>
			...
		};
		co_return factories[counter.index](tasks);
	}

	export
	template <awaitable... Ts>
		requires (sizeof...(Ts) > 0)
	auto when_any(Ts... awaitables)
	{
		return when_any_impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(awaitables)...);
	}
}
