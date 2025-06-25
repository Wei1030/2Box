export module Scheduler:ExecutionContext;
import :EventLoop;

import std;
import Coroutine;

namespace sched
{
	export
	template <typename EventLoopT>
	struct Scheduler
	{
		EventLoopT* loop;

		explicit Scheduler(EventLoopT* inLoop) noexcept : loop{inLoop}
		{
		}

		using Task = std::move_only_function<void()>;

		void addTask(Task task)
		{
			loop->addTask(std::move(task));
		}

		void addTimer(std::chrono::steady_clock::time_point expireTime, Task task)
		{
			loop->addTimer(expireTime, std::move(task));
		}

		void addTimer(std::chrono::steady_clock::time_point expireTime, Task task, std::stop_token cancellationToken)
		{
			loop->addTimer(expireTime, std::move(task), std::move(cancellationToken));
		}

		template <typename Rep, typename Period>
		void addTimer(std::chrono::duration<Rep, Period> duration, Task task)
		{
			loop->addTimer(duration, std::move(task));
		}

		void addTimer(unsigned int milliseconds, Task task)
		{
			loop->addTimer(milliseconds, std::move(task));
		}

		template <typename Rep, typename Period>
		void addTimer(std::chrono::duration<Rep, Period> duration, Task task, std::stop_token cancellationToken)
		{
			loop->addTimer(duration, std::move(task), std::move(cancellationToken));
		}

		void addTimer(unsigned int milliseconds, Task task, std::stop_token cancellationToken)
		{
			loop->addTimer(milliseconds, std::move(task), std::move(cancellationToken));
		}

		template <typename Rep, typename Period>
		void addPeriodicTimer(std::chrono::duration<Rep, Period> duration, Task task)
		{
			loop->addPeriodicTimer(duration, std::move(task));
		}

		void addPeriodicTimer(unsigned int milliseconds, Task task)
		{
			loop->addPeriodicTimer(milliseconds, std::move(task));
		}

		template <typename Rep, typename Period>
		void addPeriodicTimer(std::chrono::duration<Rep, Period> duration, Task task, std::stop_token cancellationToken)
		{
			loop->addPeriodicTimer(duration, std::move(task), std::move(cancellationToken));
		}

		void addPeriodicTimer(unsigned int milliseconds, Task task, std::stop_token cancellationToken)
		{
			loop->addPeriodicTimer(milliseconds, std::move(task), std::move(cancellationToken));
		}
	};

	export class ThreadContext
	{
	public:
		explicit ThreadContext(unsigned int initialThreadCount = std::thread::hardware_concurrency() * 2)
		{
			for (unsigned int i = 0; i < initialThreadCount; ++i)
			{
				m_threads.emplace_back([this]()
				{
					m_eventLoop.run();
				});
			}
		}

		~ThreadContext()
		{
			m_eventLoop.finish();
		}

		auto get_scheduler() const noexcept
		{
			return Scheduler{&m_eventLoop};
		}

	private:
		mutable EventLoop m_eventLoop{};
		std::vector<std::jthread> m_threads;
	};

	export class SingleThreadContext final : public ThreadContext
	{
	public:
		SingleThreadContext() : ThreadContext(1)
		{
		}
	};

	export
	template <typename ExecutionContext>
		requires requires(ExecutionContext ctx)
		{
			{ ctx.get_scheduler().addTask(std::declval<std::coroutine_handle<>>()) };
		}
	coro::LazyTask<void> transfer_to(const ExecutionContext& context)
	{
		struct TransferAwaiter
		{
			const ExecutionContext* ctx;

			bool await_ready() noexcept
			{
				return false;
			}

			void await_suspend(std::coroutine_handle<> cont)
			{
				ctx->get_scheduler().addTask(cont);
			}

			void await_resume()
			{
			}
		};
		co_return co_await TransferAwaiter{&context};
	}

	export
	template <typename SchedulerT>
#ifndef __INTELLISENSE__
		requires requires(SchedulerT sch)
		{
			{ sch.addTask(std::declval<std::coroutine_handle<>>()) };
		}
#endif
	coro::LazyTask<void> transfer_to(SchedulerT scheduler)
	{
		struct TransferAwaiter
		{
			SchedulerT* sch;

			bool await_ready() noexcept
			{
				return false;
			}

			void await_suspend(std::coroutine_handle<> cont)
			{
				sch->addTask(cont);
			}

			void await_resume()
			{
			}
		};
		co_return co_await TransferAwaiter{&scheduler};
	}
}
