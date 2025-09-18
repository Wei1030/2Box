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

		template <typename Rep, typename Period>
		void addTimer(std::chrono::duration<Rep, Period> duration, Task task, std::stop_token cancellationToken)
		{
			loop->addTimer(duration, std::move(task), std::move(cancellationToken));
		}

		template <typename Rep, typename Period>
		void addPeriodicTimer(std::chrono::duration<Rep, Period> duration, Task task)
		{
			loop->addPeriodicTimer(duration, std::move(task));
		}

		template <typename Rep, typename Period>
		void addPeriodicTimer(std::chrono::duration<Rep, Period> duration, Task task, std::stop_token cancellationToken)
		{
			loop->addPeriodicTimer(duration, std::move(task), std::move(cancellationToken));
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
			return Scheduler<EventLoop>{&m_eventLoop};
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
	template <typename T>
	concept scheduler_can_transfer_to = requires(T sch)
	{
		sch.addTask(std::declval<std::coroutine_handle<>>());
	};

	//注意: 如果被取消，则执行异常流程的线程*不一定*属于指定的scheduler
	export
	template <scheduler_can_transfer_to SchedulerT>
	coro::LazyTask<void> transfer_to(SchedulerT scheduler)
	{
		std::stop_token token = co_await coro::get_current_cancellation_token();
		if (token.stop_possible())
		{
			coro::GuaranteedResolver<void> resolver;
			coro::SharedTask<void> transferring = coro::SharedTask<void>::create([&](const coro::GuaranteedResolver<void>& res)
			{
				resolver = res;
				if (!token.stop_requested())
				{
					scheduler.addTask([resolver]
					{
						resolver->resolve();
					});
				}
			});
			std::stop_callback cb{token, [resolver] { resolver->cancel(); }};
			co_await transferring;
		}
		else
		{
			struct TransferAwaiter : std::suspend_always
			{
				SchedulerT* sch;

				void await_suspend(std::coroutine_handle<> cont)
				{
					sch->addTask(cont);
				}
			};
			co_await TransferAwaiter{.sch = &scheduler};
		}
		co_return;
	}

	export
	template <typename T>
	concept exec_ctx_can_transfer_to = requires(T ctx)
	{
		ctx.get_scheduler().addTask(std::declval<std::coroutine_handle<>>());
	};

	//注意: 如果被取消，则执行异常流程的线程*不一定*属于指定的ExecutionContext
	export
	template <exec_ctx_can_transfer_to ExecutionContext>
	coro::LazyTask<void> transfer_to(const ExecutionContext& context)
	{
		return transfer_to(context.get_scheduler());
	}

	export
	template <typename T, typename Rep, typename Period>
	concept scheduler_can_transfer_after = requires(T sch)
	{
		sch.addTimer(std::declval<std::chrono::duration<Rep, Period>>(), std::declval<std::move_only_function<void()>>());
		sch.addTimer(std::declval<std::chrono::duration<Rep, Period>>(), std::declval<std::move_only_function<void()>>(), std::declval<std::stop_token>());
	};

	//注意: 如果被取消，则执行异常流程的线程*不一定*属于指定的scheduler
	export
	template <typename Rep, typename Period, scheduler_can_transfer_after<Rep, Period> SchedulerT>
	coro::LazyTask<void> transfer_after(std::chrono::duration<Rep, Period> duration, SchedulerT scheduler)
	{
		std::stop_token token = co_await coro::get_current_cancellation_token();
		if (token.stop_possible())
		{
			coro::GuaranteedResolver<void> resolver;
			coro::SharedTask<void> transferring = coro::SharedTask<void>::create([&](const coro::GuaranteedResolver<void>& res)
			{
				resolver = res;
				if (!token.stop_requested())
				{
					scheduler.addTimer(duration, [resolver]()
					{
						resolver->resolve();
					}, token);
				}
			});
			std::stop_callback cb{token, [resolver] { resolver->cancel(); }};
			co_await transferring;
		}
		else
		{
			struct TimerAwaiter : std::suspend_always
			{
				SchedulerT* sch;
				std::chrono::duration<Rep, Period> duration;

				void await_suspend(std::coroutine_handle<> cont)
				{
					sch->addTimer(duration, cont);
				}
			};
			co_await TimerAwaiter{.sch = &scheduler, .duration = duration};
		}
		co_return;
	}

	export
	template <typename T, typename Rep, typename Period>
	concept exec_ctx_can_transfer_after = requires(T ctx)
	{
		ctx.get_scheduler().addTimer(std::declval<std::chrono::duration<Rep, Period>>(), std::declval<std::move_only_function<void()>>());
		ctx.get_scheduler().addTimer(std::declval<std::chrono::duration<Rep, Period>>(), std::declval<std::move_only_function<void()>>(), std::declval<std::stop_token>());
	};

	//注意: 如果被取消，则执行异常流程的线程*不一定*属于指定的ExecutionContext
	export
	template <typename Rep, typename Period, exec_ctx_can_transfer_after<Rep, Period> ExecutionContext>
	coro::LazyTask<void> transfer_after(std::chrono::duration<Rep, Period> duration, const ExecutionContext& context)
	{
		return transfer_after(duration, context.get_scheduler());
	}
}
