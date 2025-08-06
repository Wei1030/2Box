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

		class ProcessWrapper : public NonCopyOrMovable
		{
		public:
			explicit ProcessWrapper(DWORD pid)
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

			~ProcessWrapper()
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


		ULONGLONG get_kernel32_address_by_create_thread(const ProcessWrapper& process, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter)
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

	export
	template <ArchBit BitType = CURRENT_ARCH_BIT>
	struct InjectionDllsT
	{
		pe::MemoryModule& memModule32;
	};

	export
	template <>
	struct InjectionDllsT<ArchBit::Bit64>
	{
		pe::MemoryModule& memModule32;
		pe::MemoryModule& memModule64;
	};

	export
	using InjectionDlls = InjectionDllsT<CURRENT_ARCH_BIT>;

	namespace detail
	{
		template <ArchBit BitType = CURRENT_ARCH_BIT>
		pe::MemoryModule& select_memory_module(bool bIs32BitProcess, InjectionDllsT<BitType> dlls)
		{
			if constexpr (BitType == ArchBit::Bit64)
			{
				return bIs32BitProcess ? dlls.memModule32 : dlls.memModule64;
			}
			else
			{
				if (!bIs32BitProcess)
				{
					throw std::runtime_error{"32bit process can't inject dll to 64bit process"};
				}
				return dlls.memModule32;
			}
		}
	}

	export void inject_memory_dll_to_process(std::uint32_t dwProcessId, InjectionDlls dlls, const EssentialData& essentialData)
	{
		const detail::ProcessWrapper process{dwProcessId};

		const pe::MemoryModule& memModule = detail::select_memory_module(process.is32Bit(), dlls);
		const auto& memModuleParser = memModule.getParser();
		// 1.先将 dll文件+ ReflectiveInjectParams 整个拷贝到目标进程
		detail::VirtualMemWrapper fileMemory{process.allocMemory(memModule.getSizeOfImage() + sizeof(ReflectiveInjectParams), MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE)};
		fileMemory.write(0, memModule.getBaseAddr(), memModule.getSizeOfImage());
		fileMemory.flushInstructionCache();

		// 2.开始准备ReflectiveInjectParams
		ReflectiveInjectParams injectParams;
		// 先初始化injectionInfo
		DllInjectionInfo& injectionCtx = injectParams.injectionInfo;

		PTHREAD_START_ROUTINE loadSelfFuncAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(fileMemory.getPtr() + memModuleParser.getProcRVA("load_self"));
		injectionCtx.kernelDllAddress = detail::get_kernel32_address_by_create_thread(process, loadSelfFuncAddress, nullptr);
		// 再次将dll拷贝到目标进程，这次才是真正作为内存dll
		detail::VirtualMemWrapper dllMemory{process.allocMemory(memModule.getSizeOfImage(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE)};
		dllMemory.write(0, memModule.getBaseAddr(), memModule.getSizeOfImage());
		injectionCtx.dllAddress = reinterpret_cast<ULONGLONG>(dllMemory.getPtr());
		injectionCtx.dllSize = memModule.getSizeOfImage();
		memModuleParser.processOnOpHeader([&injectionCtx](const auto& opHeader)
		{
			injectionCtx.rvaRelocation = opHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
			injectionCtx.rvaImportDir = opHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
			injectionCtx.desiredImageBase = opHeader.ImageBase;
			injectionCtx.rvaEntryPoint = opHeader.AddressOfEntryPoint;
		});
		// 然后初始化essentialData， 直接copy过去就行
		memcpy(&injectParams.essentialData, &essentialData, sizeof(EssentialData));
		// 准备完ReflectiveInjectParams之后将其写入目标进程
		fileMemory.write(memModule.getSizeOfImage(), &injectParams, sizeof(ReflectiveInjectParams));
		void* injectParamsInRemote = fileMemory.getPtr() + memModule.getSizeOfImage();

		// 3.创建远程线程，这次将injectParams作为线程参数
		process.createThread(loadSelfFuncAddress, injectParamsInRemote);

		// 4.再次创建远程线程，初始化业务
		PTHREAD_START_ROUTINE initializeFuncAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(dllMemory.getPtr() + memModuleParser.getProcRVA("initialize"));
		process.createThread(initializeFuncAddress, injectParamsInRemote);

		// 全部成功完成，远程内存dllMemory不能释放
		dllMemory.detach();
	}
}
