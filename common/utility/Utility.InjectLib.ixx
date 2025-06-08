export module Utility.InjectLib;

import std;
import "sys_defs.h";
import Utility.MemDll;
import Utility.PEParser;
import Utility.Toolhelp;

namespace utils
{
	export struct ReflectiveInjectParams
	{
		unsigned long long kernel32BaseAddress;
		unsigned int kernel32Size;
		unsigned long long dllFileAddress;
		unsigned int dllFileSize;
	};

	export inline void injectMemoryDllToProcess(std::uint32_t dwProcessId, const char* sRawDll, std::uint32_t dwSize)
	{
		struct ResGuard
		{
			HANDLE hProcess{nullptr};
			HANDLE hThread{nullptr};

			~ResGuard()
			{
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
		ResGuard resGuard;
		resGuard.hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | // Required by Alpha
		                                PROCESS_CREATE_THREAD | // For CreateRemoteThread
		                                PROCESS_VM_OPERATION | // For VirtualAllocEx/VirtualFreeEx
		                                PROCESS_VM_WRITE | // For WriteProcessMemory,
		                                PROCESS_VM_READ,
		                                FALSE, dwProcessId);
		if (resGuard.hProcess == nullptr)
		{
			throw std::runtime_error(std::format("OpenProcess fail, error:{}", GetLastError()));
		}

		struct RemoteMemOperator
		{
			HANDLE hProcess{nullptr};
			bool bDetached{false};

			void* allocate(size_t size) const
			{
				return VirtualAllocEx(hProcess, nullptr, size + sizeof(ReflectiveInjectParams), MEM_COMMIT | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
			}

			void deallocate(void* ptr) const
			{
				if (!bDetached)
				{
					VirtualFreeEx(hProcess, ptr, 0, MEM_RELEASE);
				}
			}

			void* copy(void* dst, void const* src, size_t size) const
			{
				if (!WriteProcessMemory(hProcess, dst, src, size, nullptr))
				{
					throw std::runtime_error(std::format("WriteProcessMemory fail, error:{}", GetLastError()));
				}
				return dst;
			}

			void detach()
			{
				bDetached = true;
			}
		};
		MemDll remoteMemDll(sRawDll, dwSize, RemoteMemOperator{resGuard.hProcess});

		const PEParser parser(sRawDll, dwSize);
		DWORD loadSelfFuncOffset = parser.getProcOffset([](std::string_view funcName)
		{
			return funcName == "load_self";
		}, PEParserFlag::forceReturnRVA);

		PTHREAD_START_ROUTINE loadSelfFuncAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(remoteMemDll.data() + loadSelfFuncOffset);
		// 先以null为参数创建远程线程，目的是让目标进程加载kernel32.dll
		resGuard.hThread = CreateRemoteThread(resGuard.hProcess, nullptr, 0, loadSelfFuncAddress, nullptr, 0, nullptr);
		if (resGuard.hThread == nullptr)
		{
			throw std::runtime_error(std::format("CreateRemoteThread fail, error:{}", GetLastError()));
		}
		// Wait for the remote thread to terminate
		WaitForSingleObject(resGuard.hThread, INFINITE);
		CloseHandle(resGuard.hThread);
		resGuard.hThread = nullptr;

		// 枚举目标进程模块，找到kernel32基址
		const CToolhelp thModules(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, dwProcessId);
		MODULEENTRY32W me = {sizeof(me)};
		if (!thModules.moduleFind(L"kernel32.dll", &me))
		{
			throw std::runtime_error(std::format("Module find fail, error:{}", GetLastError()));
		}

		ReflectiveInjectParams injectParams;
		injectParams.kernel32BaseAddress = reinterpret_cast<unsigned long long>(me.modBaseAddr);
		injectParams.kernel32Size = me.modBaseSize;
		injectParams.dllFileAddress = reinterpret_cast<unsigned long long>(remoteMemDll.data());
		injectParams.dllFileSize = remoteMemDll.size();
		void* injectParamsInRemote = remoteMemDll.data() + remoteMemDll.size();
		if (!WriteProcessMemory(resGuard.hProcess, injectParamsInRemote, &injectParams, sizeof(ReflectiveInjectParams), nullptr))
		{
			throw std::runtime_error(std::format("WriteProcessMemory fail, error:{}", GetLastError()));
		}
		// 再次创建远程线程，这次将injectParams作为线程参数
		resGuard.hThread = CreateRemoteThread(resGuard.hProcess, nullptr, 0, loadSelfFuncAddress, injectParamsInRemote, 0, nullptr);
		if (resGuard.hThread == nullptr)
		{
			throw std::runtime_error(std::format("CreateRemoteThread fail, error:{}", GetLastError()));
		}
		// Wait for the remote thread to terminate
		WaitForSingleObject(resGuard.hThread, INFINITE);
		// detach,不要销毁, 因为远程进程之后要用
		remoteMemDll.getMemOperator().detach();
	}
}
