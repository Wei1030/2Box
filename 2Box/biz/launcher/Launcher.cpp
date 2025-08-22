module Launcher;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import PELoader;
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
			const std::shared_ptr<Env> env = EnvManager::instance().testFindFirstOrCreate();

			PROCESS_INFORMATION procInfo = {nullptr};
			STARTUPINFOW startupInfo = {sizeof(startupInfo)};
			startupInfo.dwFlags = STARTF_USESHOWWINDOW;
			startupInfo.wShowWindow = SW_SHOW;

			if (!DetourCreateProcessWithDllExW(nullptr, exePath.data(), nullptr, nullptr, 0,
			                                   CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, nullptr,
			                                   std::filesystem::path{exePath}.parent_path().native().c_str(), &startupInfo, &procInfo,
			                                   get_detours_injection_dll_name(env->getPath(), env->getFlagName()).c_str(), &::CreateProcessW))
			{
				throw std::runtime_error(std::format("CreateProcessW Failed, error code: {}", GetLastError()));
			}
			try
			{
				// inject_memory_dll_to_process(procInfo.dwProcessId, env.get(), get_injection_dlls(), get_essential_data());
				const std::wstring_view envPath = env->getPath();
				const std::uint32_t envPathCount = static_cast<std::uint32_t>(envPath.length());
				const std::uint32_t envPathSize = envPathCount * sizeof(wchar_t);
				const std::uint32_t paramsSize = sizeof(DetourInjectParams) + envPathSize;
				std::vector<std::byte> buffer(paramsSize);
				DetourInjectParams* injectParams = reinterpret_cast<DetourInjectParams*>(buffer.data());
				injectParams->version = get_essential_data().version;
				injectParams->envFlag = env->getFlag();
				injectParams->envPathCount = envPathCount;
				memcpy(injectParams->envPath, envPath.data(), envPathSize);
				if (!DetourCopyPayloadToProcess(procInfo.hProcess, DETOUR_INJECT_PARAMS_GUID, injectParams, paramsSize))
				{
					throw std::runtime_error(std::format("copy payload failed, error code: {}", GetLastError()));
				}
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
