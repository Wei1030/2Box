export module Utility.InjectLib;

import std;
import "sys_defs.h";
import Utility.PEParser;
import Utility.Toolhelp;

namespace utils
{
	export inline void injectMemoryDllToProcess(std::uint32_t dwProcessId, const char* sRawDll, std::uint32_t dwSize)
	{
		struct Res
		{
			HANDLE hProcess{nullptr};
			char* fileRemoteAddr{nullptr};
			HANDLE hThread{nullptr};

			~Res()
			{
				if (fileRemoteAddr)
				{
					VirtualFreeEx(hProcess, fileRemoteAddr, 0, MEM_RELEASE);
				}
				if (hThread)
				{
					CloseHandle(hThread);
				}
				if (hProcess)
				{
					CloseHandle(hProcess);
				}
			}
		};
		Res res;
		res.hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | // Required by Alpha
		                           PROCESS_CREATE_THREAD | // For CreateRemoteThread
		                           PROCESS_VM_OPERATION | // For VirtualAllocEx/VirtualFreeEx
		                           PROCESS_VM_WRITE | // For WriteProcessMemory,
		                           PROCESS_VM_READ,
		                           FALSE, dwProcessId);
		if (res.hProcess == nullptr)
		{
			throw std::runtime_error(std::format("OpenProcess fail, error:{}", GetLastError()));
		}

		res.fileRemoteAddr = static_cast<char*>(VirtualAllocEx(res.hProcess, nullptr, dwSize, MEM_COMMIT | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE));
		if (res.fileRemoteAddr == nullptr)
		{
			throw std::runtime_error(std::format("VirtualAllocEx fail, error:{}", GetLastError()));
		}

		if (!WriteProcessMemory(res.hProcess, res.fileRemoteAddr, sRawDll, dwSize, nullptr))
		{
			throw std::runtime_error(std::format("WriteProcessMemory fail, error:{}", GetLastError()));
		}

		const PEParser parser(sRawDll, dwSize);
		DWORD loadSelfFuncOffset = parser.getProcFOA([](std::string_view funcName)
		{
			return funcName == "load_self";
		});

		PTHREAD_START_ROUTINE loadSelfFuncAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(res.fileRemoteAddr + loadSelfFuncOffset);
		// 先以null为参数创建远程线程，目的是让目标进程加载kernel32.dll
		res.hThread = CreateRemoteThread(res.hProcess, nullptr, 0, loadSelfFuncAddress, nullptr, 0, nullptr);
		if (res.hThread == nullptr)
		{
			throw std::runtime_error(std::format("CreateRemoteThread fail, error:{}", GetLastError()));
		}
		// Wait for the remote thread to terminate
		WaitForSingleObject(res.hThread, INFINITE);
		CloseHandle(res.hThread);
		res.hThread = nullptr;

		// 枚举目标进程模块，找到kernel32基址
		const CToolhelp thModules(TH32CS_SNAPMODULE, dwProcessId);
		MODULEENTRY32W me = {sizeof(me)};
		if (!thModules.moduleFind(L"kernel32.dll", &me))
		{
			throw std::runtime_error(std::format("Module find fail, error:{}", GetLastError()));
		}
		return;
	}
}
