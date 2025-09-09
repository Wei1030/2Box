export module WndEnumerator;

import "sys_defs.h";
import std;
import Scheduler;
import Coroutine;
import Env;

namespace biz
{
	export class WndEnumerator
	{
	public:
		void requestGetProcessAllWnd(DWORD pid, const std::shared_ptr<Env>& env);

		~WndEnumerator();
	private:
		coro::LazyTask<void> addEnumTask(DWORD pid, std::shared_ptr<Env> env);
		coro::LazyTask<void> enumAllWindow();

	private:
		std::unordered_map<DWORD, std::shared_ptr<Env>> m_requests;
		sched::SingleThreadContext m_execCtx;
		std::stop_source m_stopSource{std::nostopstate};
		coro::AsyncScope m_asyncScope;
	};
}
