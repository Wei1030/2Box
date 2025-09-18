module WndEnumerator;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif
import Biz.Core;

namespace
{
	BOOL CALLBACK my_enum_windows_proc(HWND hWnd, LPARAM lParam)
	{
		std::unordered_map<DWORD, std::shared_ptr<biz::Env>>& requests = *reinterpret_cast<std::unordered_map<DWORD, std::shared_ptr<biz::Env>>*>(lParam);
		DWORD pid{0};
		if (GetWindowThreadProcessId(hWnd, &pid))
		{
			const auto it = requests.find(pid);
			if (it != requests.end())
			{
				it->second->addToplevelWindow(pid, hWnd);
			}
		}
		return TRUE;
	}
}

namespace biz
{
	void WndEnumerator::requestGetProcessAllWnd(DWORD pid, const std::shared_ptr<Env>& env)
	{
		m_asyncScope.spawn(addEnumTask(pid, env));
	}

	WndEnumerator::~WndEnumerator()
	{
		m_stopSource.request_stop();
		m_asyncScope.join();
	}

	coro::LazyTask<void> WndEnumerator::addEnumTask(DWORD pid, std::shared_ptr<Env> env)
	{
		co_await sched::transfer_to(m_execCtx.get_scheduler());
		// 合并所有2s内的请求
		m_requests.insert(std::make_pair(pid, env));
		m_stopSource.request_stop();
		m_stopSource = std::stop_source();
		m_asyncScope.spawn(coro::co_with_cancellation(enumAllWindow(), m_stopSource.get_token()));
	}

	coro::LazyTask<void> WndEnumerator::enumAllWindow()
	{
		co_await sched::transfer_after(std::chrono::seconds(2), m_execCtx.get_scheduler());
		// 2s 内如果没有被取消才开始枚举符合条件的窗口
		EnumWindows(&my_enum_windows_proc, reinterpret_cast<LPARAM>(&m_requests));
		m_requests.clear();
	}
}
