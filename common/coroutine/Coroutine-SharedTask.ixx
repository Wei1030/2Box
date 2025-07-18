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
		              || (std::is_object_v<T> && std::is_copy_constructible_v<T>),
		              "SharedTask's first template argument must be void, a reference type, or a "
		              "copy-constructible object type");

		using value_type = T;

		struct promise_type final : public SharedTaskPromise<T>
		{
			SharedTask get_return_object() noexcept
			{
				return SharedTask{this};
			}
		};

		SharedTask() = delete;

		template <typename E>
			requires requires(const E& func)
			{
				func(GuaranteedResolver<T>{});
			}
		static SharedTask create(const E& executor)
		{
			struct ExecAwaiter
			{
				const E& exec;
				GuaranteedResolver<T> resolver{new Resolver<T>};

				bool await_ready() noexcept
				{
					return false;
				}

				void await_suspend(std::coroutine_handle<promise_type> cont)
				{
					resolver->setNextCoroutineHandle(cont);
					auto resolveGuard = resolver;
					try
					{
						exec(resolveGuard);
					}
					catch (...)
					{
						resolveGuard->reject(std::current_exception());
					}
				}

				decltype(auto) await_resume()
				{
					return resolver->getValue();
				}
			};

			co_return co_await ExecAwaiter{executor};
		}

		static SharedTask reject(std::exception_ptr e)
		{
			std::rethrow_exception(e);
			// ReSharper disable CppUnreachableCode
			std::unreachable();
			// 为了让这个函数成为协程,必须有协程相关关键字
			co_return Resolver<T>{}.getValue();
			// ReSharper restore CppUnreachableCode
		}

		struct AwaiterBase
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
		};

		struct CopyValueAwaiter : public AwaiterBase
		{
			decltype(auto) await_resume()
			{
				return AwaiterBase::promisePtr->copyValue();
			}
		};

		// 默认都用copy, 不管左右值,都不能假设SharedTask的引用情况, 用copy最安全
		CopyValueAwaiter operator co_await()
		{
			return CopyValueAwaiter{m_p};
		}

		struct RefValueAwaiter : public AwaiterBase
		{
			decltype(auto) await_resume()
			{
				return AwaiterBase::promisePtr->getValue();
			}
		};

		// 左值上允许获取value的引用，用户自行保证SharedTask没有全部被销毁
		RefValueAwaiter getValueRef() &
		{
			return RefValueAwaiter{m_p};
		}

		struct MoveValueAwaiter : public AwaiterBase
		{
			decltype(auto) await_resume()
			{
				return static_cast<promise_type&&>(*AwaiterBase::promisePtr).getValue();
			}
		};

		// 右值上允许移动value, 用户自行保证其他SharedTask已经全部被销毁
		MoveValueAwaiter moveValue() &&
			requires (std::move_constructible<T>)
		{
			return MoveValueAwaiter{m_p};
		}

		void waitUntilDone() noexcept
		{
			SyncLatch syncLatch;
			[](SharedTask& self, SyncLatch& latch) noexcept -> OnewayTask
			{
				ArriveOnExit guard(latch);
				co_await RefValueAwaiter{self.m_p};
			}(*this, syncLatch);
			syncLatch.wait();
		}

		decltype(auto) syncAwait()
		{
			waitUntilDone();
			return m_p->getValue();
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
	template <typename T>
	SharedTask<T> start_and_shared(LazyTask<T> lazy)
	{
		co_return co_await lazy;
	}
}
