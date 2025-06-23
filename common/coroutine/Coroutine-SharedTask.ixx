export module Coroutine:SharedTask;

import :PromiseBase;
import :LazyTask;
import std;

namespace coro
{
	export
	template <typename T>
	class SharedTask;

	struct SharedTaskWaiter
	{
		std::coroutine_handle<> continuation{nullptr};
		SharedTaskWaiter* next{nullptr};
	};

	template <typename T>
	class SharedTaskPromise : public PromiseTmplBase<T>
	{
	public:
		template <typename>
		friend class SharedTask;

		void addRef() noexcept
		{
			m_refCount.fetch_add(1, std::memory_order_relaxed);
		}

		void release() noexcept
		{
			if (m_refCount.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				abandon();
			}
		}

		void abandon() noexcept
		{
			void* const valueAbandoned = &this->m_waiters;
			const void* flag = m_waiters.exchange(valueAbandoned, std::memory_order_acq_rel);
			const void* const doneFlag = this;
			if (flag == doneFlag)
			{
				std::coroutine_handle<SharedTaskPromise>::from_promise(*this).destroy();
			}
		}

		void finish()
		{
			void* const doneFlag = this;
			const void* const abandonedFlag = &this->m_waiters;
			void* waiters = m_waiters.exchange(doneFlag, std::memory_order_acq_rel);
			if (waiters == abandonedFlag)
			{
				std::coroutine_handle<SharedTaskPromise>::from_promise(*this).destroy();
			}
			else if (waiters != nullptr)
			{
				const SharedTaskWaiter* waiter = static_cast<SharedTaskWaiter*>(waiters);
				while (waiter->next != nullptr)
				{
					// 先保存next,因为continuation()之后waiter可能已经被销毁
					const SharedTaskWaiter* next = waiter->next;
					waiter->continuation.resume();
					waiter = next;
				}
				// 最后一个
				waiter->continuation.resume();
			}
		}

		// ReSharper disable CppMemberFunctionMayBeStatic

		// 已经有LazyTask了， SharedTask设计成开始即运行
		std::suspend_never initial_suspend() noexcept
		{
			return {};
		}

		struct FinalAwaiter
		{
			bool await_ready() noexcept
			{
				return false;
			}

			template <typename Promise>
			void await_suspend(std::coroutine_handle<Promise> coro) noexcept
			{
				coro.promise().finish();
			}

			void await_resume() noexcept
			{
			}
		};

		FinalAwaiter final_suspend() noexcept
		{
			return {};
		}

		// ReSharper restore CppMemberFunctionMayBeStatic

		bool isReady() const noexcept
		{
			const void* const doneFlag = this;
			return m_waiters.load(std::memory_order_acquire) == doneFlag;
		}

		bool tryAwait(SharedTaskWaiter* waiter) noexcept
		{
			const void* const doneFlag = this;
			void* oldWaiters = m_waiters.load(std::memory_order_acquire);

			do
			{
				if (oldWaiters == doneFlag)
				{
					// Coroutine already completed, don't suspend.
					return false;
				}

				waiter->next = static_cast<SharedTaskWaiter*>(oldWaiters);
			}
			while (!m_waiters.compare_exchange_weak(
				oldWaiters,
				waiter,
				std::memory_order_release,
				std::memory_order_acquire));
			return true;
		}

	private:
		std::atomic<std::uint32_t> m_refCount{0};

		// Value is either
		// - nullptr          - indicates started, no waiters
		// - &this->m_waiters - abandoned
		// - this             - indicates value is ready
		// - other            - pointer to head item in linked-list of waiters.
		//                      values are of type 'shared_task_waiter'.
		std::atomic<void*> m_waiters{nullptr};
	};

	template <typename T>
	class SharedTask
	{
	public:
		static_assert(std::is_void_v<T> || std::is_reference_v<T>
		              || (std::is_object_v<T> && std::is_move_constructible_v<T>),
		              "SharedTask's first template argument must be void, a reference type, or a "
		              "move-constructible object type");

		using value_type = T;

		struct promise_type final : public SharedTaskPromise<T>
		{
			SharedTask get_return_object() noexcept
			{
				return SharedTask{this};
			}
		};

		SharedTask() = delete;

		struct Awaiter
		{
			PromisePtr<promise_type> promisePtr;
			SharedTaskWaiter waiter{};

			bool await_ready() const noexcept
			{
				return promisePtr->isReady();
			}

			bool await_suspend(std::coroutine_handle<> awaiter) noexcept
			{
				waiter.continuation = awaiter;
				return promisePtr->tryAwait(&waiter);
			}

			decltype(auto) await_resume()
			{
				// 是否考虑当promise引用只剩下我这一个的时候，把值移动出去?
				// 这种情况因为引用在这内部，计数必不可能再增加了，应该没有什么竞争了?
				// static_cast<promise_type&&>(*promisePtr).getValue();
				return promisePtr->getValue();
			}
		};

		Awaiter operator co_await()
		{
			return Awaiter{m_p};
		}

		constexpr explicit operator bool() const noexcept
		{
			return static_cast<bool>(m_p);
		}

		[[nodiscard]] constexpr bool operator==(const SharedTask& other) noexcept
		{
			return m_p.get() == other.m_p.get();
		}

	private:
		friend class SharedTaskPromise<T>;

		explicit SharedTask(promise_type* ptr) noexcept
			: m_p(ptr)
		{
		}

		PromisePtr<promise_type> m_p;
	};

	export
	template <typename Func, typename... Args>
	auto spawn(Func&& func, Args&&... args) -> SharedTask<typename std::invoke_result_t<Func, Args...>::value_type>
	{
		co_return co_await std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	export
	template <typename T>
	SharedTask<T> start_and_shared(LazyTask<T> lazy)
	{
		co_return co_await lazy;
	}
}
