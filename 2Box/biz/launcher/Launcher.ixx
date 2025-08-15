export module Launcher;

import std;
import Scheduler;
import Coroutine;

namespace biz
{
	export class Launcher
	{
	public:
		static Launcher& instance()
		{
			static Launcher instance;
			return instance;
		}

	public:
		void run(std::wstring_view exePath);

	private:
		coro::LazyTask<void> launch(std::wstring exePath);
	private:
		Launcher() = default;

	private:
		sched::SingleThreadContext m_execCtx;
	};
}
