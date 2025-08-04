export module PELoader:Exceptions;

import "sys_defs.h";
import std;
import :SystemInfo;
import :Symbol;
import :Loader;

namespace pe
{
	namespace detail
	{
		bool insert_inverted_function_table(const MemoryModule& memoryModule)
		{
			HMODULE ntdllAddress = GetModuleHandleW(L"ntdll.dll");
			if (!ntdllAddress)
			{
				return false;
			}
#ifdef _WIN64
			if (g_sym_rva.RtlInsertInvertedFunctionTable64 == 0)
			{
				return false;
			}
			const std::uint64_t pfnRtlInsertInvertedFunctionTable = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_sym_rva.RtlInsertInvertedFunctionTable64;
#else
			if (g_sym_rva.RtlInsertInvertedFunctionTable32 == 0)
			{
				return false;
			}
			const std::uint64_t pfnRtlInsertInvertedFunctionTable = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_sym_rva.RtlInsertInvertedFunctionTable32;
#endif

			if (g_os_version.isWindows8Point1OrGreater)
			{
				using RtlInsertInvertedFunctionTablePtr = NTSTATUS(__fastcall *)(PVOID, ULONG);
				reinterpret_cast<RtlInsertInvertedFunctionTablePtr>(pfnRtlInsertInvertedFunctionTable)(const_cast<char*>(memoryModule.getBaseAddr()), memoryModule.getSizeOfImage());
			}
			else if (g_os_version.isWindows8OrGreater)
			{
				using RtlInsertInvertedFunctionTablePtr = NTSTATUS(__stdcall *)(PVOID, ULONG);
				reinterpret_cast<RtlInsertInvertedFunctionTablePtr>(pfnRtlInsertInvertedFunctionTable)(const_cast<char*>(memoryModule.getBaseAddr()), memoryModule.getSizeOfImage());
			}
			else
			{
#ifdef _WIN64
				if (g_sym_rva.LdrpInvertedFunctionTable64 == 0)
				{
					return false;
				}
				const std::uint64_t pfnLdrpInvertedFunctionTable = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_sym_rva.LdrpInvertedFunctionTable64;
#else
				if (g_sym_rva.LdrpInvertedFunctionTable32 == 0)
				{
					return false;
				}
				const std::uint64_t pfnLdrpInvertedFunctionTable = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_sym_rva.LdrpInvertedFunctionTable32;
#endif

				using RtlInsertInvertedFunctionTablePtr = NTSTATUS(__stdcall *)(PVOID, PVOID, ULONG);
				reinterpret_cast<RtlInsertInvertedFunctionTablePtr>(pfnRtlInsertInvertedFunctionTable)(reinterpret_cast<void*>(pfnLdrpInvertedFunctionTable),
				                                                                                       const_cast<char*>(memoryModule.getBaseAddr()), memoryModule.getSizeOfImage());
			}
			return true;
		}
	}

	// 启用异常需要满足前提：编译选项必须启用SEH异常 (/EHa), 并且32位版本还必须启用 /SAFESEH
	// 否则异常功能还是会不正常.懒得研究了, 反正只是用来加载自己的特定的dll而已
	export bool enable_exceptions(const MemoryModule& memoryModule)
	{
		return detail::insert_inverted_function_table(memoryModule);
	}
}
