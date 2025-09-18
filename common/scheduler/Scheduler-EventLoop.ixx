export module Scheduler:EventLoop;

import std;
import "sys_defs.h";
import :TimedQueue;

namespace sched
{
	template <typename DerivedT>
	class EventLoopBase
	{
	public:
		using Task = std::move_only_function<void()>;
		using TaskList = std::list<Task>;

		void addTask(Task task)
		{
			{
				std::lock_guard guard(m_queueLock);
				m_timeQueue.addTask(std::move(task));
			}
			static_cast<DerivedT*>(this)->notify();
		}

		void addTimer(std::chrono::steady_clock::time_point expireTime, Task task)
		{
			{
				std::lock_guard guard(m_queueLock);
				m_timeQueue.addTimer(expireTime, std::move(task));
			}
			static_cast<DerivedT*>(this)->notify();
		}

		void addTimer(std::chrono::steady_clock::time_point expireTime, Task task, std::stop_token cancellationToken)
		{
			{
				std::lock_guard guard(m_queueLock);
				m_timeQueue.addTimerWithCancellation(expireTime, std::move(task), std::move(cancellationToken));
			}
			static_cast<DerivedT*>(this)->notify();
		}

		template <typename Rep, typename Period>
		void addTimer(std::chrono::duration<Rep, Period> duration, Task task)
		{
			{
				std::lock_guard guard(m_queueLock);
				m_timeQueue.addTimer(duration, std::move(task));
			}
			static_cast<DerivedT*>(this)->notify();
		}

		template <typename Rep, typename Period>
		void addTimer(std::chrono::duration<Rep, Period> duration, Task task, std::stop_token cancellationToken)
		{
			{
				std::lock_guard guard(m_queueLock);
				m_timeQueue.addTimerWithCancellation(duration, std::move(task), std::move(cancellationToken));
			}
			static_cast<DerivedT*>(this)->notify();
		}

		template <typename Rep, typename Period>
		void addPeriodicTimer(std::chrono::duration<Rep, Period> duration, Task task)
		{
			{
				auto periodicTimer = [this, duration, task = std::move(task)]() mutable
				{
					task();
					addPeriodicTimer(duration, std::move(task));
				};
				std::lock_guard guard(m_queueLock);
				m_timeQueue.addTimer(duration, std::move(periodicTimer));
			}
			static_cast<DerivedT*>(this)->notify();
		}

		template <typename Rep, typename Period>
		void addPeriodicTimer(std::chrono::duration<Rep, Period> duration, Task task, std::stop_token cancellationToken)
		{
			{
				auto periodicTimer = [this, duration, task = std::move(task), token = cancellationToken]() mutable
				{
					// 能触发就说明刚刚判断过stop_requested,这里没必要再次判断
					task();
					// 执行完任务后需要再次判断
					if (!token.stop_requested())
					{
						addPeriodicTimer(duration, std::move(task), std::move(token));
					}
				};
				std::lock_guard guard(m_queueLock);
				m_timeQueue.addTimerWithCancellation(duration, std::move(periodicTimer), std::move(cancellationToken));
			}
			static_cast<DerivedT*>(this)->notify();
		}

	protected:
		std::mutex m_queueLock{};
		MultiTimingWheel m_timeQueue{};
	};

	export class EventLoopForWinUi : public EventLoopBase<EventLoopForWinUi>
	{
	public:
		EventLoopForWinUi()
		{
			m_hNotifyEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
			if (!m_hNotifyEvent)
			{
				throw std::runtime_error("CreateEventW failed");
			}
		}

		~EventLoopForWinUi()
		{
			CloseHandle(m_hNotifyEvent);
		}

		void run()
		{
			MSG msg;
			TaskList tasks;
			// 只能通过WM_QUIT窗口消息来结束run
			while (true)
			{
				handleTasks(tasks);

				// 优先处理完所有窗口消息
				while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
				{
					if (msg.message == WM_QUIT)
					{
						break;
					}
					TranslateMessage(&msg);
					DispatchMessageW(&msg);
				}

				for (auto it = tasks.begin(); it != tasks.end();)
				{
					if (auto& func = *it)
					{
						func();
					}
					it = tasks.erase(it);
				}

				if (msg.message == WM_QUIT)
				{
					return;
				}
			}
		}

		void finish() const noexcept
		{
			PostQuitMessage(0);
			SetEvent(m_hNotifyEvent);
		}

	private:
		friend EventLoopBase;

		void handleTasks(TaskList& out)
		{
			std::unique_lock lock(m_queueLock);

			DWORD dwMilliseconds = INFINITE;
			const auto [bHasTask, tp] = m_timeQueue.nextTaskTimePoint();
			if (bHasTask)
			{
				auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(tp - std::chrono::steady_clock::now()).count();
				if (delay < 0)
				{
					delay = 0;
				}
				dwMilliseconds = static_cast<DWORD>(delay);
			}

			lock.unlock();
			const DWORD result = MsgWaitForMultipleObjects(1, &m_hNotifyEvent, FALSE, dwMilliseconds,QS_ALLINPUT);
			lock.lock();

			if (result == WAIT_TIMEOUT)
			{
				m_timeQueue.advanceUntil(tp);
			}
			out = m_timeQueue.pull();
		}

		void notify() const
		{
			SetEvent(m_hNotifyEvent);
		}

	private:
		HANDLE m_hNotifyEvent;
	};

	export class EventLoop : public EventLoopBase<EventLoop>
	{
	public:
		void run()
		{
			TaskList tasks;
			bool bContinue = true;
			while (bContinue)
			{
				bContinue = handleTasks(tasks);

				for (auto it = tasks.begin(); it != tasks.end();)
				{
					if (auto& func = *it)
					{
						func();
					}
					it = tasks.erase(it);
				}
			}
		}

		bool handleTasks(TaskList& out)
		{
			std::lock_guard guard(m_queueLock);
			if (m_bStopped)
			{
				return false;
			}
			const auto [bHasTask, tp] = m_timeQueue.nextTaskTimePoint();
			if (bHasTask)
			{
				if (m_cv.wait_until(m_queueLock, tp) == std::cv_status::timeout)
				{
					m_timeQueue.advanceUntil(tp);
				}
			}
			else
			{
				m_cv.wait(m_queueLock);
			}
			out = m_timeQueue.pull();
			return !m_bStopped;
		}

		void finish()
		{
			std::lock_guard guard(m_queueLock);
			m_bStopped = true;
			m_cv.notify_all();
		}

	private:
		friend EventLoopBase;

		void notify()
		{
			m_cv.notify_one();
		}

	private:
		std::condition_variable_any m_cv{};
		bool m_bStopped = false;
	};
}
