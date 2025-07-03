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
	template <typename SchedulerT>
		requires requires(SchedulerT sch)
		{
			{ sch.addTask(std::declval<std::coroutine_handle<>>()) };
		}
	coro::LazyTask<void> transfer_to(SchedulerT scheduler)
	{
		struct TransferAwaiter : std::suspend_always
		{
			SchedulerT* sch;

			void await_suspend(std::coroutine_handle<> cont)
			{
				sch->addTask(cont);
			}
		};
		co_return co_await TransferAwaiter{.sch = &scheduler};
	}

	export
	template <typename ExecutionContext>
#ifndef __INTELLISENSE__
		requires requires(ExecutionContext ctx)
		{
			{ ctx.get_scheduler() };
		}
#endif
	coro::LazyTask<void> transfer_to(const ExecutionContext& context)
	{
		return transfer_to(context.get_scheduler());
	}


	export
	template <typename Rep, typename Period, typename SchedulerT>
		requires requires(SchedulerT sch)
		{
			{ sch.addTimer(std::declval<std::chrono::duration<Rep, Period>>(), std::declval<std::move_only_function<void()>>()) };
			{ sch.addTimer(std::declval<std::chrono::duration<Rep, Period>>(), std::declval<std::move_only_function<void()>>(), std::declval<std::stop_token>()) };
		}
	coro::LazyTask<void> transfer_after(std::chrono::duration<Rep, Period> duration, SchedulerT scheduler)
	{
		std::stop_token token = co_await coro::get_current_cancellation_token();
		if (token.stop_possible())
		{
			union cancellation_guard
			{
				struct StopNotify
				{
					coro::LazyTask<void>::Resolver resolver;

					void operator()() const noexcept
					{
						resolver->reject(std::make_exception_ptr(std::runtime_error("operation canceled")));
					}
				};

				std::stop_callback<StopNotify> callback;

				void setupCancellationHandler(const std::stop_token& token, const coro::LazyTask<void>::Resolver& resolver)
				{
					std::construct_at(&callback, token, resolver);
				}

				// 不初始化std::stop_callback
				cancellation_guard()
				{
				}

				~cancellation_guard()
				{
					std::destroy_at(&callback);
				}
			};
			// 该协程, 超时和取消都需要能够返回, 需要一个std::stop_callback注册取消回调,取消后需要调用resolver的接口reject掉这个task
			// 但是resolver只能在coro::LazyTask::create的回调里才能拿到,在这个回调里构造std::stop_callback的话,无法保持其生命周期,它又不能移动出来
			// 因此用一个union包装一下，相当于延迟构造一下std::stop_callback
			cancellation_guard guard;
			// std::stop_callback有可能已经stop而在构造中直接调用callback,
			// 即reject直接在setupCancellationHandler中被调用导致这个coro::LazyTask<void>::create出来的task直接结束,如果直接co_await它,相当于本协程直接被恢复执行而结束,从而析构掉std::stop_callback
			// 这就相当于std::stop_callback构造都还未结束而析构了...
			// 要解决这个问题需要将LazyTask转成SharedTask,让其直接启动,这样一来,如果真的立马同步式的结束了,这意味着还没有人正在co_await,就不会恢复本协程,从而有机会让std::stop_callback从构造中先返回出来
			co_await coro::start_and_shared(coro::LazyTask<void>::create([&](const coro::LazyTask<void>::Resolver& resolver)
			{
				guard.setupCancellationHandler(token, resolver);
				if (!token.stop_requested())
				{
					scheduler.addTimer(duration, [resolver]()
					{
						resolver->resolve();
					}, token);
				}
			}));
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
	template <typename SchedulerT>
		requires (!requires(SchedulerT sch) { sch.get_scheduler(); })
		&& (requires(SchedulerT sch) { transfer_after(std::chrono::milliseconds{}, sch); })
	coro::LazyTask<void> transfer_after(unsigned int milliseconds, SchedulerT scheduler)
	{
		return transfer_after(std::chrono::milliseconds{milliseconds}, std::move(scheduler));
	}

	export
	template <typename Rep, typename Period, typename ExecutionContext>
#ifndef __INTELLISENSE__
		requires requires(ExecutionContext ctx)
		{
			{ ctx.get_scheduler() };
		}
#endif
	coro::LazyTask<void> transfer_after(std::chrono::duration<Rep, Period> duration, const ExecutionContext& context)
	{
		return transfer_after(duration, context.get_scheduler());
	}

	export
	template <typename ExecutionContext>
#ifndef __INTELLISENSE__
		requires requires(ExecutionContext ctx)
		{
			{ ctx.get_scheduler() };
		}
#endif
	coro::LazyTask<void> transfer_after(unsigned int milliseconds, const ExecutionContext& context)
	{
		return transfer_after(std::chrono::milliseconds{milliseconds}, context.get_scheduler());
	}
}
