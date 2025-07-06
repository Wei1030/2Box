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
			: counter(count)

		{
		}

		void startWait(std::coroutine_handle<> next)
		{
			cont = next;
		}

		void finishAndResumeNextIfNeed(std::size_t n) noexcept
		{
			// 不是首个完成的任务
			if (settled.exchange(true, std::memory_order_relaxed))
			{
				return;
			}

			// 首个完成的任务
			index = n;
			cont.resume();
		}

		// 因异常而结束
		void finishAndResumeNextIfNeed(std::size_t n, const std::exception_ptr&) noexcept
		{
			if (counter.fetch_sub(1, std::memory_order_relaxed) == 1)
			{
				// 全部任务都异常了
				index = n;
				cont.resume();
			}
		}

		void addRef() noexcept
		{
			refCounter.fetch_add(1, std::memory_order_relaxed);
		}

		void release()
		{
			if (refCounter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				delete this;
			}
		}

		std::atomic<std::size_t> refCounter{0};
		std::atomic<std::size_t> counter;
		std::coroutine_handle<> cont{};

		std::atomic<bool> settled{false};
		std::size_t index{0};
	};

	using WhenAnyCtrlBlock = PromisePtr<WhenAnyCounter>;

	template <typename T>
	class WhenAnyPerTaskPromise : public PromiseTmplBase<T>
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
				m_ctrlBlock->finishAndResumeNextIfNeed(m_index, PromiseTmplBase<T>::except);
			}
			else
			{
				m_ctrlBlock->finishAndResumeNextIfNeed(m_index);
			}
			release();
		}

		auto final_suspend() noexcept
		{
			struct FinalAwaiter
			{
				WhenAnyPerTaskPromise& self;

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

		void setCtrlBlockAndIndex(const WhenAnyCtrlBlock& ctrlBlock, std::size_t index) noexcept
		{
			m_ctrlBlock = ctrlBlock;
			m_index = index;
		}

		void release() noexcept
		{
			if (m_abandoned.exchange(true, std::memory_order_acq_rel))
			{
				std::coroutine_handle<WhenAnyPerTaskPromise>::from_promise(*this).destroy();
			}
		}

	private:
		std::atomic<bool> m_abandoned{false};
		std::stop_token m_token;
		WhenAnyCtrlBlock m_ctrlBlock{nullptr};
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
				m_coro.promise().release();
			}
		}

		WhenAnyPerTask& operator=(const WhenAnyPerTask&) = delete;
		WhenAnyPerTask& operator=(WhenAnyPerTask&& that) = delete;

		void setCtrlBlockAndIndex(const WhenAnyCtrlBlock& ctrlBlock, std::size_t index) noexcept
		{
			m_coro.promise().setCtrlBlockAndIndex(ctrlBlock, index);
		}

		void start()
		{
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
			: m_ctrlBlock(new WhenAnyCounter{sizeof...(Tasks)})
			  , m_tasks(std::move(tasks)...)
		{
		}

		explicit WhenAnyHelper(std::tuple<Tasks...>&& tasks)
			: m_ctrlBlock(new WhenAnyCounter{sizeof...(Tasks)})
			  , m_tasks(std::move(tasks))
		{
		}

		WhenAnyCtrlBlock& getCtrlBlock() noexcept
		{
			return m_ctrlBlock;
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
					self.getCtrlBlock()->startWait(cont);
					self.startTasks(std::make_integer_sequence<std::size_t, sizeof...(Tasks)>{});
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
			(std::get<Is>(m_tasks).setCtrlBlockAndIndex(m_ctrlBlock, Is), ...);
			// 再全部启动
			(std::get<Is>(m_tasks).start(), ...);
		}

	private:
		WhenAnyCtrlBlock m_ctrlBlock;
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

		constexpr std::array factories{
			&make_when_any_result_helper<Is, Ts...>
			...
		};
		co_return factories[whenAnyHelper.getCtrlBlock()->index](whenAnyHelper.getTasks());
	}

	export
	template <awaitable... Ts>
		requires (sizeof...(Ts) > 0)
	auto when_any(Ts... awaitables)
	{
		return when_any_impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(awaitables)...);
	}
}
