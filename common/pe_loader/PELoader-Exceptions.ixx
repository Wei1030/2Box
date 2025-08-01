export module PELoader:Exceptions;

import "sys_defs.h";
import std;
import :Loader;

namespace pe
{
	bool insert_inverted_function_table(const MemoryModule& memoryModule, OsVersionInfo info)
	{
		HMODULE ntdllAddress = GetModuleHandleW(L"ntdll.dll");
#ifdef _WIN64
		std::uint64_t rtlInsertInvertedFunctionTable = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_symbols.rvaRtlInsertInvertedFunctionTable64;
		std::uint64_t ldrpInvertedFunctionTable = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_symbols.rvaLdrpInvertedFunctionTable64;
#else
		std::uint64_t rtlInsertInvertedFunctionTable = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_symbols.rvaRtlInsertInvertedFunctionTable32;
		std::uint64_t ldrpInvertedFunctionTable = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_symbols.rvaLdrpInvertedFunctionTable32;
#endif
		if (rtlInsertInvertedFunctionTable == 0 || ldrpInvertedFunctionTable == 0)
		{
			return false;
		}
		
		if (info.isWindows8Point1OrGreater)
		{
			
			using RtlInsertInvertedFunctionTableWin81 = int (__fastcall *)(std::uint64_t, std::uint32_t);
			reinterpret_cast<RtlInsertInvertedFunctionTableWin81>(rtlInsertInvertedFunctionTable)(reinterpret_cast<std::uint64_t>(memoryModule.getBaseAddr()), memoryModule.getSizeOfImage());
			
		}
		else if (info.isWindows8OrGreater)
		{
			using RtlInsertInvertedFunctionTableWin8 = int (__stdcall *)(std::uint64_t, std::uint32_t);
			reinterpret_cast<RtlInsertInvertedFunctionTableWin8>(rtlInsertInvertedFunctionTable)(reinterpret_cast<std::uint64_t>(memoryModule.getBaseAddr()), memoryModule.getSizeOfImage());
		}
		else
		{
			using RtlInsertInvertedFunctionTable = int(__stdcall *)(std::uint64_t, std::uint64_t, std::uint32_t);
			reinterpret_cast<RtlInsertInvertedFunctionTable>(rtlInsertInvertedFunctionTable)(ldrpInvertedFunctionTable,
			                                                                                 reinterpret_cast<std::uint64_t>(memoryModule.getBaseAddr()), memoryModule.getSizeOfImage());
		}
		return true;
	}

	export bool enable_exceptions(const MemoryModule& memoryModule, OsVersionInfo info)
	{
		return insert_inverted_function_table(memoryModule, info);
	}
}
