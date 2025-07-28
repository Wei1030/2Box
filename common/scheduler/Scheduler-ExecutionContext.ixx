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

	namespace detail
	{
		// 由于resolver只能在coro::LazyTask::create的回调里才能拿到,在回调里构造std::stop_callback的话,无法保持其生命周期,它又不能移动出来
		// 因此引入这个类用一个union包装一下std::stop_callback，相当于延迟构造一下std::stop_callback
		struct CancellationHandler
		{
		private:
			struct StopNotify
			{
				coro::GuaranteedResolver<void> resolver;

				void operator()() const noexcept
				{
					resolver->cancel();
				}
			};

			union
			{
				std::stop_callback<StopNotify> callback;
			};

			bool m_bSetup{false};

		public:
			void setupHandler(const std::stop_token& token, const coro::GuaranteedResolver<void>& resolver)
			{
				std::construct_at(&callback, token, StopNotify{resolver});
				m_bSetup = true;
			}

			// 不初始化std::stop_callback
			// ReSharper disable once CppPossiblyUninitializedMember
			CancellationHandler() noexcept
			{
			}

			~CancellationHandler()
			{
				if (m_bSetup)
				{
					std::destroy_at(&callback);
				}
			}
		};
	}

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
			detail::CancellationHandler handler;
			// std::stop_callback有可能已经stop而在构造中直接调用callback,
			// 即reject直接在setupHandler中被调用导致这个coro::LazyTask<void>::create出来的task直接结束,如果直接co_await它,相当于本协程直接被恢复执行而结束,从而析构掉std::stop_callback
			// 这就相当于std::stop_callback构造都还未结束而析构了...
			// 要解决这个问题必须用SharedTask::create,让其直接启动,这样一来,如果真的立马同步式的结束了,这意味着还没有人正在co_await,就不会恢复本协程,从而有机会让std::stop_callback从构造中先返回出来
			co_await coro::SharedTask<void>::create([&](const coro::GuaranteedResolver<void>& resolver)
			{
				handler.setupHandler(token, resolver);
				if (!token.stop_requested())
				{
					scheduler.addTask([resolver]
					{
						resolver->resolve();
					});
				}
				else
				{
					resolver->cancel();
				}
			});
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
			detail::CancellationHandler handler;
			// std::stop_callback有可能已经stop而在构造中直接调用callback,
			// 即reject直接在setupHandler中被调用导致这个coro::LazyTask<void>::create出来的task直接结束,如果直接co_await它,相当于本协程直接被恢复执行而结束,从而析构掉std::stop_callback
			// 这就相当于std::stop_callback构造都还未结束而析构了...
			// 要解决这个问题必须用SharedTask::create,让其直接启动,这样一来,如果真的立马同步式的结束了,这意味着还没有人正在co_await,就不会恢复本协程,从而有机会让std::stop_callback从构造中先返回出来
			co_await coro::SharedTask<void>::create([&](const coro::GuaranteedResolver<void>& resolver)
			{
				handler.setupHandler(token, resolver);
				if (!token.stop_requested())
				{
					scheduler.addTimer(duration, [resolver]()
					{
						resolver->resolve();
					}, token);
				}
				else
				{
					resolver->cancel();
				}
			});
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
