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
			: counter(count)
		{
		}

		enum class Status : std::uint32_t
		{
			NoWaiter = 0,
			HasWaiter,
			Done
		};

		bool startWait(std::coroutine_handle<> next) noexcept
		{
			cont = next;

			Status expected = Status::NoWaiter;
			return status.compare_exchange_strong(expected, Status::HasWaiter, std::memory_order_acq_rel);
		}

		void finishAndResumeNextIfNeed() noexcept
		{
			if (counter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				// 全部任务都完成了
				const Status lastStatus = status.exchange(Status::Done, std::memory_order_acq_rel);
				if (lastStatus == Status::HasWaiter)
				{
					cont.resume();
				}
			}
		}

		// 因异常而结束
		void finishAndResumeNextIfNeed(const std::exception_ptr& e) noexcept
		{
			// 不是首个异常的任务
			if (bError.exchange(true, std::memory_order_relaxed))
			{
				return;
			}

			// 首个异常的任务
			exception = e;
			const Status lastStatus = status.exchange(Status::Done, std::memory_order_acq_rel);
			if (lastStatus == Status::HasWaiter)
			{
				cont.resume();
			}
		}

		void addRef() noexcept
		{
			refCounter.fetch_add(1, std::memory_order_relaxed);
		}

		void release() noexcept
		{
			if (refCounter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				delete this;
			}
		}

		std::atomic<std::size_t> refCounter{0};
		std::atomic<std::size_t> counter;

		std::atomic<bool> bError{false};
		std::atomic<Status> status{Status::NoWaiter};
		std::coroutine_handle<> cont{};
		std::exception_ptr exception{};
	};

	using WhenAllCtrlBlock = PromisePtr<WhenAllCounter>;

	template <typename T>
	class WhenAllPerTaskPromise : public PromiseTmplBase<T>
	{
	public:
		std::suspend_always initial_suspend() noexcept
		{
			return {};
		}

		void finish() noexcept
		{
			if (PromiseBase::m_disc == PromiseBase::Discriminator::Exception)
			{
				m_ctrlBlock->finishAndResumeNextIfNeed(PromiseTmplBase<T>::except);
			}
			else
			{
				m_ctrlBlock->finishAndResumeNextIfNeed();
			}
			release();
		}

		auto final_suspend() noexcept
		{
			struct FinalAwaiter
			{
				WhenAllPerTaskPromise& self;

				bool await_ready() const noexcept { return false; }

				void await_suspend(std::coroutine_handle<>) const noexcept
				{
					self.finish();
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

		void setCtrlBlock(const WhenAllCtrlBlock& ctrlBlock) noexcept
		{
			m_ctrlBlock = ctrlBlock;
		}

		void release() noexcept
		{
			if (m_abandoned.exchange(true, std::memory_order_acq_rel))
			{
				std::coroutine_handle<WhenAllPerTaskPromise>::from_promise(*this).destroy();
			}
		}

	private:
		std::atomic<bool> m_abandoned{false};
		std::stop_token m_token;
		WhenAllCtrlBlock m_ctrlBlock{nullptr};
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
				m_coro.promise().release();
			}
		}

		WhenAllPerTask& operator=(const WhenAllPerTask&) = delete;
		WhenAllPerTask& operator=(WhenAllPerTask&& that) = delete;

		void setCtrlBlock(const WhenAllCtrlBlock& ctrlBlock) noexcept
		{
			m_coro.promise().setCtrlBlock(ctrlBlock);
		}

		void start()
		{
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
			: m_ctrlBlock(new WhenAllCounter{sizeof...(Tasks)})
			  , m_tasks(std::move(tasks)...)
		{
		}

		explicit WhenAllHelper(std::tuple<Tasks...>&& tasks)
			: m_ctrlBlock(new WhenAllCounter{sizeof...(Tasks)})
			  , m_tasks(std::move(tasks))
		{
		}

		WhenAllCtrlBlock& getCtrlBlock() noexcept
		{
			return m_ctrlBlock;
		}

		std::tuple<Tasks...>& getTasks() noexcept
		{
			return m_tasks;
		}

		auto waitAllDoneOrAnyError() noexcept
		{
			struct Awaiter
			{
				WhenAllHelper& self;
				bool await_ready() const noexcept { return false; }

				bool await_suspend(std::coroutine_handle<> cont) noexcept
				{
					self.startTasks(std::make_integer_sequence<std::size_t, sizeof...(Tasks)>{});
					return self.getCtrlBlock()->startWait(cont);
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
			// 先传递block到所有task中,加完引用计数
			(std::get<Is>(m_tasks).setCtrlBlock(m_ctrlBlock), ...);
			// 再全部启动
			(std::get<Is>(m_tasks).start(), ...);
		}

	private:
		WhenAllCtrlBlock m_ctrlBlock;
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

		co_await whenAllHelper.waitAllDoneOrAnyError();

		if (whenAllHelper.getCtrlBlock()->exception)
		{
			cancelSrc.request_stop();
			std::rethrow_exception(whenAllHelper.getCtrlBlock()->exception);
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
