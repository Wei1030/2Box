export module Injector;

import std;
import "sys_defs.h";
import DynamicWin32Api;
import PELoader;
import Utility.Toolhelp;

namespace injector
{
	namespace detail
	{
		class NonCopyOrMovable
		{
		public:
			NonCopyOrMovable() = default;

			NonCopyOrMovable(const NonCopyOrMovable&) = delete;
			NonCopyOrMovable& operator=(const NonCopyOrMovable&) = delete;
			NonCopyOrMovable(NonCopyOrMovable&&) = delete;
			NonCopyOrMovable& operator=(NonCopyOrMovable&&) = delete;
		};

		class VirtualMemWrapper : public NonCopyOrMovable
		{
		public:
			VirtualMemWrapper(HANDLE hProcess, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect)
				: m_hProcess(hProcess)
				  , m_size(dwSize)
			{
				m_pAddress = VirtualAllocEx(hProcess, nullptr, dwSize, flAllocationType, flProtect);
				if (!m_pAddress)
				{
					throw std::runtime_error(std::format("VirtualAllocEx fail, error:{}", GetLastError()));
				}
			}

			~VirtualMemWrapper()
			{
				if (m_pAddress)
				{
					VirtualFreeEx(m_hProcess, m_pAddress, 0, MEM_RELEASE);
				}
			}

			void write(SIZE_T offset, LPCVOID lpBuffer, SIZE_T nSize) const
			{
				if (!WriteProcessMemory(m_hProcess, getPtr() + offset, lpBuffer, nSize, nullptr))
				{
					throw std::runtime_error(std::format("WriteProcessMemory fail, error:{}", GetLastError()));
				}
			}

			void flushInstructionCache() const
			{
				FlushInstructionCache(m_hProcess, m_pAddress, m_size);
			}

			DWORD protect(DWORD flNewProtect) const
			{
				DWORD oldProtect;
				VirtualProtectEx(m_hProcess, m_pAddress, m_size, flNewProtect, &oldProtect);
				return oldProtect;
			}

		public:
			char* getPtr() const
			{
				return static_cast<char*>(m_pAddress);
			}

			void detach()
			{
				m_pAddress = nullptr;
			}

		private:
			HANDLE m_hProcess;
			void* m_pAddress;
			SIZE_T m_size;
		};

		class ThreadWrapper : public NonCopyOrMovable
		{
		public:
			ThreadWrapper(HANDLE hProcess, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter)
			{
				m_hThread = CreateRemoteThread(hProcess, nullptr, 0, lpStartAddress, lpParameter, 0, nullptr);
				if (!m_hThread)
				{
					throw std::runtime_error(std::format("CreateRemoteThread fail, error:{}", GetLastError()));
				}
			}

			~ThreadWrapper()
			{
				if (m_hThread)
				{
					WaitForSingleObject(m_hThread, INFINITE);
					CloseHandle(m_hThread);
				}
			}

		private:
			HANDLE m_hThread;
		};

		class ProcessHandleWrapper : public NonCopyOrMovable
		{
		public:
			explicit ProcessHandleWrapper(DWORD pid)
				: m_dwProcessId(pid)
			{
				m_hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | // Required by Alpha
				                         PROCESS_CREATE_THREAD | // For CreateRemoteThread
				                         PROCESS_VM_OPERATION | // For VirtualAllocEx/VirtualFreeEx
				                         PROCESS_VM_WRITE | // For WriteProcessMemory,
				                         PROCESS_VM_READ,
				                         FALSE, m_dwProcessId);
				if (!m_hProcess)
				{
					throw std::runtime_error(std::format("OpenProcess fail, error:{}", GetLastError()));
				}
			}

			~ProcessHandleWrapper()
			{
				if (m_hProcess)
				{
					CloseHandle(m_hProcess);
				}
			}

		public:
			VirtualMemWrapper allocMemory(SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) const
			{
				return {m_hProcess, dwSize, flAllocationType, flProtect};
			}

			ThreadWrapper createThread(LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter) const
			{
				return {m_hProcess, lpStartAddress, lpParameter};
			}

			bool is32Bit() const
			{
				if (win32_api::IsWow64Process2)
				{
					USHORT processMachine;
					USHORT nativeMachine;
					if (!win32_api::IsWow64Process2(m_hProcess, &processMachine, &nativeMachine))
					{
						throw std::runtime_error(std::format("IsWow64Process2 fail, error:{}", GetLastError()));
					}
					if (processMachine != IMAGE_FILE_MACHINE_UNKNOWN)
					{
						return true;
					}
					// return nativeMachine != IMAGE_FILE_MACHINE_IA64 && nativeMachine != IMAGE_FILE_MACHINE_AMD64 && nativeMachine != IMAGE_FILE_MACHINE_ARM64;
					// or
					return pe::g_os_version.is32BitSystem;
				}

				if (win32_api::IsWow64Process)
				{
					BOOL bIsWow64;
					if (!win32_api::IsWow64Process(m_hProcess, &bIsWow64))
					{
						throw std::runtime_error(std::format("IsWow64Process fail, error:{}", GetLastError()));
					}
					if (bIsWow64)
					{
						return true;
					}
					return pe::g_os_version.is32BitSystem;
				}
				return true;
			}

		public:
			DWORD pid() const noexcept { return m_dwProcessId; }
			HANDLE handle() const noexcept { return m_hProcess; }

		private:
			DWORD m_dwProcessId;
			HANDLE m_hProcess;
		};


		ULONGLONG get_kernel32_address_by_create_thread(const ProcessHandleWrapper& process, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter)
		{
			// step 1:
			// 创建远程线程，只是为了让目标进程加载kernel32.dll
			process.createThread(lpStartAddress, lpParameter);
			// 返回值ThreadWrapper析构时会自动等待线程退出
			// step 2:
			// 枚举目标进程模块，找到kernel32基址
			const utils::CToolhelp thModules(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, process.pid());
			MODULEENTRY32W me = {sizeof(me)};
			if (!thModules.moduleFind(L"kernel32.dll", &me))
			{
				throw std::runtime_error(std::format("Module find fail, error:{}", GetLastError()));
			}
			return reinterpret_cast<ULONGLONG>(me.modBaseAddr);
		}
	}

	export void inject_memory_dll_to_process(std::uint32_t dwProcessId, const pe::MemoryModule& module, const EssentialData& essentialData)
	{
		const detail::ProcessHandleWrapper process{dwProcessId};
#ifndef _WIN64
		if (!process.is32Bit())
		{
			// TODO: 32位进程无法为64位进程创建远程线程，这种情况则 rpc 到主进程帮忙注入
			return;
		}
#endif
		// 1.先将dll文件整个拷贝到目标进程, 主要用于当目标进程启动子进程时注入子进程。注入就需要知道dll所在
		detail::VirtualMemWrapper fileMemory{process.allocMemory(module.getSizeOfImage() + sizeof(ReflectiveInjectParams), MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE)};
		fileMemory.write(0, module.getBaseAddr(), module.getSizeOfImage());
		fileMemory.flushInstructionCache();

		// 2.开始准备ReflectiveInjectParams
		ReflectiveInjectParams injectParams;
		// 先初始化injectionInfo
		DllInjectionInfo& injectionCtx = injectParams.injectionInfo;
		const pe::Parser<pe::parser_flag::HasSectionAligned>& parser = module.getParser();
		PTHREAD_START_ROUTINE loadSelfFuncAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(fileMemory.getPtr() + parser.getProcRVA("load_self"));
		injectionCtx.kernelDllAddress = detail::get_kernel32_address_by_create_thread(process, loadSelfFuncAddress, nullptr);
		injectionCtx.fileAddress = reinterpret_cast<ULONGLONG>(fileMemory.getPtr());
		injectionCtx.fileSize = module.getSizeOfImage();
		// 再次将dll拷贝到目标进程，这次才是真正作为内存dll
		detail::VirtualMemWrapper dllMemory{process.allocMemory(module.getSizeOfImage(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)};
		dllMemory.write(0, module.getBaseAddr(), module.getSizeOfImage());
		injectionCtx.dllAddress = reinterpret_cast<ULONGLONG>(dllMemory.getPtr());
		injectionCtx.dllSize = module.getSizeOfImage();
		parser.processOnOpHeader([&injectionCtx](const auto& opHeader)
		{
			injectionCtx.rvaRelocation = opHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
			injectionCtx.rvaImportDir = opHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
			injectionCtx.desiredImageBase = opHeader.ImageBase;
			injectionCtx.rvaEntryPoint = opHeader.AddressOfEntryPoint;
		});
		// 然后初始化essentialData， 直接copy过去就行
		memcpy(&injectParams.essentialData, &essentialData, sizeof(EssentialData));
		// 准备完ReflectiveInjectParams之后将其写入目标进程
		fileMemory.write(module.getSizeOfImage(), &injectParams, sizeof(ReflectiveInjectParams));
		void* injectParamsInRemote = fileMemory.getPtr() + module.getSizeOfImage();
		
		// 3.创建远程线程，这次将injectParams作为线程参数
		process.createThread(loadSelfFuncAddress, injectParamsInRemote);
		// 远程线程执行结束后可以把fileMemory的保护属性设置为 READONLY 了
		[[maybe_unused]] DWORD oldProtect = fileMemory.protect(PAGE_READONLY);

		// 4.再次创建远程线程，初始化业务
		PTHREAD_START_ROUTINE initializeFuncAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(dllMemory.getPtr() + parser.getProcRVA("initialize"));
		process.createThread(initializeFuncAddress, injectParamsInRemote);

		// 全部成功完成，则不需要释放远程内存
		fileMemory.detach();
		dllMemory.detach();
	}
}
