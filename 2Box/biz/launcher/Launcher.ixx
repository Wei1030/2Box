export module Launcher;

import std;
import Scheduler;
import Coroutine;

namespace biz
{
	export class Launcher
	{
	public:
		void run(std::wstring_view exePath);

	private:
		coro::LazyTask<void> launch(std::wstring exePath);
	
	private:
		sched::SingleThreadContext m_execCtx;
		coro::AsyncScope m_asyncScope;
	};
}
