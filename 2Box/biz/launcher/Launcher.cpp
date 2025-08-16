module Launcher;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import PELoader;
import Injector;
import EssentialData;
import Env;

namespace biz
{
	void Launcher::run(std::wstring_view exePath)
	{
		launch(std::wstring{exePath.data(), exePath.size()}).detachAndStart();
	}

	coro::LazyTask<void> Launcher::launch(std::wstring exePath)
	{
		try
		{
			co_await sched::transfer_to(m_execCtx);
			std::shared_ptr<Env> env = EnvManager::instance().testFindFirstOrCreate();
			
			PROCESS_INFORMATION procInfo = {nullptr};
			STARTUPINFOW startupInfo = {sizeof(startupInfo)};
			startupInfo.dwFlags = STARTF_USESHOWWINDOW;
			startupInfo.wShowWindow = SW_SHOW;

			if (!CreateProcessW(nullptr, exePath.data(), nullptr, nullptr, 0,
			                    CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, nullptr, nullptr, &startupInfo, &procInfo))
			{
				throw std::runtime_error(std::format("CreateProcessW Failed, error code: {}", GetLastError()));
			}
			try
			{
				inject_memory_dll_to_process(procInfo.dwProcessId, env.get(), get_injection_dlls(), get_essential_data());
				ResumeThread(procInfo.hThread);
				CloseHandle(procInfo.hThread);
				CloseHandle(procInfo.hProcess);
			}
			catch (const std::exception& e)
			{
				TerminateProcess(procInfo.hProcess, 0);
				CloseHandle(procInfo.hThread);
				CloseHandle(procInfo.hProcess);
				throw std::runtime_error(std::format("注入失败，{}", e.what()));
			}
			catch (...)
			{
				TerminateProcess(procInfo.hProcess, 0);
				CloseHandle(procInfo.hThread);
				CloseHandle(procInfo.hProcess);
				throw std::runtime_error(std::format("注入失败，发生未知错误"));
			}
		}
		catch (const std::exception& e)
		{
			show_utf8_error_message(std::format("启动进程失败：{}", e.what()));
		}
		catch (...)
		{
			show_error_message(L"启动进程失败：发生未知错误");
		}
		co_return;
	}
}
