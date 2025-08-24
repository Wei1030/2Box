export module Coroutine:AsyncScope;

import std;
import :Concept;
import :PromiseBase;

namespace coro
{
	export class AsyncScope
	{
	public:
		~AsyncScope()
		{
			join();
		}

		struct FinishOnExit
		{
			AsyncScope& scope;

			explicit FinishOnExit(AsyncScope& s) noexcept : scope(s)
			{
			}

			~FinishOnExit() noexcept
			{
				scope.onWorkFinished();
			}
		};

		template <awaitable T>
		void spawn(T&& t)
		{
			[](AsyncScope& scope, std::decay_t<T> a)-> OnewayTask
			{
				scope.onWorkStarted();
				FinishOnExit guard{scope};
				co_await std::move(a);
			}(*this, std::forward<T>(t));
		}

		[[nodiscard]] auto joinAsync() noexcept
		{
			struct Awaiter
			{
				AsyncScope& scope;

				explicit Awaiter(AsyncScope& s) noexcept : scope(s)
				{
				}

				bool await_ready() const noexcept
				{
					return scope.m_count.load(std::memory_order_acquire) == 0;
				}

				bool await_suspend(std::coroutine_handle<> continuation) const noexcept
				{
					scope.m_continuation = continuation;
					return scope.m_count.fetch_sub(1, std::memory_order_acq_rel) > 1;
				}

				// ReSharper disable once CppMemberFunctionMayBeStatic
				void await_resume() noexcept
				{
				}
			};

			return Awaiter{*this};
		}

		void join() noexcept
		{
			SyncLatch syncLatch;
			[](AsyncScope& scope, SyncLatch& latch) noexcept -> OnewayTask
			{
				ArriveOnExit guard(latch);
				co_await scope.joinAsync();
			}(*this, syncLatch);
			syncLatch.wait();
		}

		// must join first
		void reset() noexcept
		{
			m_count.store(1, std::memory_order_relaxed);
		}

	private:
		void onWorkStarted() noexcept
		{
			m_count.fetch_add(1, std::memory_order_relaxed);
		}

		void onWorkFinished() noexcept
		{
			if (m_count.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				m_continuation.resume();
			}
		}

	private:
		std::atomic<std::uint32_t> m_count{1};
		std::coroutine_handle<> m_continuation{nullptr};
	};
}
