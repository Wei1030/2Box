export module Launcher;

import std;
import Scheduler;
import Coroutine;
import Env;

namespace biz
{
	export class Launcher
	{
	public:
		void run(const std::shared_ptr<Env>& env, std::wstring_view exePath, std::wstring_view params = L"");
		void runInNewEnv(std::wstring_view exePath, std::wstring_view params = L"");

		coro::LazyTask<void> coRun(std::shared_ptr<Env> env, std::wstring_view exePath, std::wstring_view params);

	private:
		coro::LazyTask<void> launch(const std::shared_ptr<Env>& env, std::wstring_view exePath, std::wstring_view params) const;
		coro::LazyTask<void> launchInternal(std::shared_ptr<Env> env, std::wstring exePath, std::wstring params) const;

	private:
		sched::SingleThreadContext m_execCtx;
		coro::AsyncScope m_asyncScope;
	};
}
