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
import Utility.SystemInfo;

namespace
{
	PROCESS_INFORMATION create_and_inject(const biz::Env* env, std::wstring_view exePath)
	{
		PROCESS_INFORMATION procInfo = {nullptr};
		STARTUPINFOW startupInfo = {sizeof(startupInfo)};
		startupInfo.dwFlags = STARTF_USESHOWWINDOW;
		startupInfo.wShowWindow = SW_HIDE;

		namespace fs = std::filesystem;
		const fs::path cmdPath{fs::weakly_canonical(fs::path{sys_info::get_system_dir()} / fs::path{L"cmd.exe"})};
		std::wstring cmdLine = std::format(LR"(/c start "" "{}")", exePath);
		if (!DetourCreateProcessWithDllExW(cmdPath.c_str(), cmdLine.data(), nullptr, nullptr, 0,
		                                   CREATE_DEFAULT_ERROR_MODE | CREATE_SUSPENDED, nullptr,
		                                   std::filesystem::path{exePath}.parent_path().native().c_str(), &startupInfo, &procInfo,
		                                   env->getDllFullPath().data(), &::CreateProcessW))
		{
			throw std::runtime_error(std::format("CreateProcessW Failed, error code: {}", GetLastError()));
		}
		try
		{
			const std::wstring_view rootPath = app().exeDir();
			const std::uint32_t rootPathCount = static_cast<std::uint32_t>(rootPath.length());
			const std::uint32_t rootPathSize = rootPathCount * sizeof(wchar_t);
			const std::uint32_t paramsSize = sizeof(DetourInjectParams) + rootPathSize;
			std::vector<std::byte> buffer(paramsSize);
			DetourInjectParams* injectParams = reinterpret_cast<DetourInjectParams*>(buffer.data());
			injectParams->version = biz::get_essential_data().version;
			injectParams->envFlag = env->getFlag();
			injectParams->envIndex = env->getIndex();
			injectParams->rootPathCount = rootPathCount;
			memcpy(injectParams->rootPath, rootPath.data(), rootPathSize);
			if (!DetourCopyPayloadToProcess(procInfo.hProcess, DETOUR_INJECT_PARAMS_GUID, injectParams, paramsSize))
			{
				throw std::runtime_error(std::format("copy payload failed, error code: {}", GetLastError()));
			}
		}
		catch (...)
		{
			TerminateProcess(procInfo.hProcess, 0);
			CloseHandle(procInfo.hThread);
			CloseHandle(procInfo.hProcess);
			throw;
		}
		return procInfo;
	}
}

namespace biz
{
	void Launcher::run(std::wstring_view exePath)
	{
		m_asyncScope.spawn(launch(std::wstring{exePath}));
	}

	coro::LazyTask<void> Launcher::launch(std::wstring exePath)
	{
		try
		{
			co_await sched::transfer_to(m_execCtx);

			const std::shared_ptr<Env> env = EnvManager::instance().testFindFirstOrCreate();
			const PROCESS_INFORMATION procInfo = create_and_inject(env.get(), exePath);
			ResumeThread(procInfo.hThread);
			CloseHandle(procInfo.hThread);
			CloseHandle(procInfo.hProcess);
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
