export module Utility.InjectLib;

import std;
import "sys_defs.h";
import PELoader;
import Utility.Toolhelp;

namespace utils
{
	struct InjectResourceGuard
	{
		HANDLE hProcess{nullptr};
		HANDLE hThread{nullptr};
		char* pFileAddress{nullptr};
		char* dllMemAddress{nullptr};

		void detachMemory()
		{
			dllMemAddress = nullptr;
		}

		~InjectResourceGuard()
		{
			if (dllMemAddress)
			{
				VirtualFreeEx(hProcess, dllMemAddress, 0, MEM_RELEASE);
			}
			if (pFileAddress)
			{
				VirtualFreeEx(hProcess, pFileAddress, 0, MEM_RELEASE);
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

	// struct MyStruct
	// {
	// 	MyStruct()
	// 	{
	// 		GetSystemWow64DirectoryW(szSysWow64Dir, MAX_PATH);
	// 		std::wstring wow64Kernel32Dir = szSysWow64Dir;
	// 		wow64Kernel32Dir += L"\\kernel32.dll";
	// 		hFile = CreateFileW(wow64Kernel32Dir.c_str(),
	// 			GENERIC_READ,
	// 			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	// 			NULL,
	// 			OPEN_EXISTING,
	// 			FILE_ATTRIBUTE_NORMAL,
	// 			NULL);
	// 		m_hFileMapping = CreateFileMappingW(hFile,
	// 			NULL,
	// 			PAGE_READONLY | SEC_IMAGE,
	// 			0, 0, NULL);
	// 		m_pMemory = MapViewOfFile(m_hFileMapping, FILE_MAP_READ, 0, 0, 0);
	// 	}
	// 	void* m_pMemory;
	// 	HANDLE m_hFileMapping{ nullptr };
	// 	HANDLE hFile{ nullptr };
	// 	wchar_t szSysWow64Dir[MAX_PATH]{ 0 };
	// };

	export inline void inject_memory_dll_to_process(std::uint32_t dwProcessId, const pe::MemoryModule& memoryModule)
	{
		// static MyStruct ss;
		// static const pe::Parser<pe::parser_flag::HasSectionAligned> kernel32Parser{ reinterpret_cast<const char*>(ss.m_pMemory) };
		static const pe::Parser<pe::parser_flag::HasSectionAligned> kernel32Parser{reinterpret_cast<const char*>(GetModuleHandleW(L"kernel32.dll"))};

		InjectResourceGuard resGuard;
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

		resGuard.pFileAddress = static_cast<char*>(VirtualAllocEx(resGuard.hProcess, nullptr, memoryModule.getSizeOfImage() + sizeof(ReflectiveInjectParams),
		                                                          MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN,
		                                                          PAGE_EXECUTE_READWRITE));
		if (resGuard.pFileAddress == nullptr)
		{
			throw std::runtime_error(std::format("VirtualAllocEx fail, error:{}", GetLastError()));
		}
		if (!WriteProcessMemory(resGuard.hProcess, resGuard.pFileAddress, memoryModule.getBaseAddr(), memoryModule.getSizeOfImage(), nullptr))
		{
			throw std::runtime_error(std::format("WriteProcessMemory fail, error:{}", GetLastError()));
		}
		FlushInstructionCache(resGuard.hProcess, nullptr, 0);

		const pe::Parser<pe::parser_flag::HasSectionAligned>& parser = memoryModule.getParser();
		PTHREAD_START_ROUTINE loadSelfFuncAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(resGuard.pFileAddress + parser.getProcRVA("load_self"));
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
		injectParams.kernel32Address = reinterpret_cast<ULONGLONG>(me.modBaseAddr);
		injectParams.loadLibraryARVA = kernel32Parser.getProcRVA("LoadLibraryA");
		injectParams.getProcAddressRVA = kernel32Parser.getProcRVA("GetProcAddress");
		injectParams.flushInstructionCacheRVA = kernel32Parser.getProcRVA("FlushInstructionCache");

		resGuard.dllMemAddress = static_cast<char*>(VirtualAllocEx(resGuard.hProcess, nullptr, memoryModule.getSizeOfImage(), MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
		if (resGuard.dllMemAddress == nullptr)
		{
			throw std::runtime_error(std::format("VirtualAllocEx fail, error:{}", GetLastError()));
		}
		if (!WriteProcessMemory(resGuard.hProcess, resGuard.dllMemAddress, memoryModule.getBaseAddr(), memoryModule.getSizeOfImage(), nullptr))
		{
			throw std::runtime_error(std::format("WriteProcessMemory fail, error:{}", GetLastError()));
		}

		injectParams.dllMemAddress = reinterpret_cast<ULONGLONG>(resGuard.dllMemAddress);
		injectParams.dllMemSize = memoryModule.getSizeOfImage();
		parser.processOnOpHeader([&injectParams](const auto& opHeader)
		{
			injectParams.dllRelocationRVA = opHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
			injectParams.dllImportDirRVA = opHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress;
			injectParams.dllImageBase = opHeader.ImageBase;
			injectParams.entryPointRVA = opHeader.AddressOfEntryPoint;
		});
		injectParams.isWindows8OrGreater = IsWindows8OrGreater();
		injectParams.isWindows8Point1OrGreater = IsWindows8Point1OrGreater();

		injectParams.rvaLdrpHandleTlsData32 = pe::g_symbols.rvaLdrpHandleTlsData32;
		injectParams.rvaLdrpHandleTlsData64 = pe::g_symbols.rvaLdrpHandleTlsData64;
		injectParams.rvaLdrpInvertedFunctionTable32 = pe::g_symbols.rvaLdrpInvertedFunctionTable32;
		injectParams.rvaLdrpInvertedFunctionTable64 = pe::g_symbols.rvaLdrpInvertedFunctionTable64;
		injectParams.rvaRtlInsertInvertedFunctionTable32 = pe::g_symbols.rvaRtlInsertInvertedFunctionTable32;
		injectParams.rvaRtlInsertInvertedFunctionTable64 = pe::g_symbols.rvaRtlInsertInvertedFunctionTable64;

		void* injectParamsInRemote = resGuard.pFileAddress + memoryModule.getSizeOfImage();
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
		// Wait for the remote thread to terminate
		WaitForSingleObject(resGuard.hThread, INFINITE);
		CloseHandle(resGuard.hThread);
		resGuard.hThread = nullptr;

		// 再次创建远程线程，这次将injectParams作为参数调用initialize
		PTHREAD_START_ROUTINE initializeFuncAddress = reinterpret_cast<PTHREAD_START_ROUTINE>(resGuard.dllMemAddress + parser.getProcRVA("initialize"));
		resGuard.hThread = CreateRemoteThread(resGuard.hProcess, nullptr, 0, initializeFuncAddress, injectParamsInRemote, 0, nullptr);
		if (resGuard.hThread == nullptr)
		{
			throw std::runtime_error(std::format("CreateRemoteThread fail, error:{}", GetLastError()));
		}
		// Wait for the remote thread to terminate
		WaitForSingleObject(resGuard.hThread, INFINITE);
		resGuard.detachMemory();
	}
}
