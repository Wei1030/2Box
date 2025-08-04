export module PELoader:StaticTLS;

import "sys_defs.h";
import std;
import :SystemInfo;
import :Symbol;
import :Loader;

namespace pe
{
	namespace detail
	{
		// ReSharper disable CppInconsistentNaming
		template <typename T>
		struct _LIST_ENTRY_T
		{
			T Flink;
			T Blink;
		};

		template <typename T>
		struct _UNICODE_STRING_T
		{
			using type = T;

			std::uint16_t Length;
			std::uint16_t MaximumLength;
			T Buffer;
		};

		template <typename T>
		struct _LDR_DATA_TABLE_ENTRY_BASE_T
		{
			_LIST_ENTRY_T<T> InLoadOrderLinks;
			_LIST_ENTRY_T<T> InMemoryOrderLinks;
			_LIST_ENTRY_T<T> InInitializationOrderLinks;
			T DllBase;
			T EntryPoint;
			std::uint32_t SizeOfImage;
			_UNICODE_STRING_T<T> FullDllName;
			_UNICODE_STRING_T<T> BaseDllName;
			std::uint32_t Flags;
			std::uint16_t LoadCount;
			std::uint16_t TlsIndex;
			_LIST_ENTRY_T<T> HashLinks;
			std::uint32_t TimeDateStamp;
			T EntryPointActivationContext;
			T PatchInformation;
		};

		using _LDR_DATA_TABLE_ENTRY_BASE32 = _LDR_DATA_TABLE_ENTRY_BASE_T<std::uint32_t>;
		using _LDR_DATA_TABLE_ENTRY_BASE64 = _LDR_DATA_TABLE_ENTRY_BASE_T<std::uint64_t>;

		// ReSharper restore CppInconsistentNaming

		bool add_static_tls_entry(const MemoryModule& memoryModule)
		{
			if (!g_os_version.isWindowsVistaOrGreater)
			{
				// 不考虑Vista以下的系统
				return false;
			}

			HMODULE ntdllAddress = GetModuleHandleW(L"ntdll.dll");
			if (!ntdllAddress)
			{
				return false;
			}
#ifdef _WIN64
			if (!g_sym_rva.LdrpHandleTlsData64)
			{
				return false;
			}
			std::uint64_t ldrpHandleTlsData = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_sym_rva.LdrpHandleTlsData64;
#else
			if (!g_sym_rva.LdrpHandleTlsData32)
			{
				return false;
			}
			std::uint64_t ldrpHandleTlsData = reinterpret_cast<std::uint64_t>(ntdllAddress) + g_sym_rva.LdrpHandleTlsData32;
#endif

#ifdef _WIN64
			_LDR_DATA_TABLE_ENTRY_BASE64* pAddr = static_cast<_LDR_DATA_TABLE_ENTRY_BASE64*>(VirtualAlloc(nullptr, sizeof(_LDR_DATA_TABLE_ENTRY_BASE64), MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE));
#else
			_LDR_DATA_TABLE_ENTRY_BASE32* pAddr = static_cast<_LDR_DATA_TABLE_ENTRY_BASE32*>(VirtualAlloc(nullptr, sizeof(_LDR_DATA_TABLE_ENTRY_BASE32), MEM_COMMIT | MEM_RESERVE,PAGE_READWRITE));
#endif
			if (!pAddr)
			{
				return false;
			}
			pAddr->DllBase = std::bit_cast<decltype(pAddr->DllBase)>(memoryModule.getBaseAddr());

			NTSTATUS status;
			if (g_os_version.isWindows8Point1OrGreater)
			{
				using LdrpHandleTlsData = NTSTATUS(__fastcall *)(decltype(pAddr));
				status = reinterpret_cast<LdrpHandleTlsData>(ldrpHandleTlsData)(pAddr);
			}
			else
			{
				using LdrpHandleTlsData = NTSTATUS(__stdcall *)(decltype(pAddr));
				status = reinterpret_cast<LdrpHandleTlsData>(ldrpHandleTlsData)(pAddr);
			}
			return NT_SUCCESS(status);
		}
	}

	export bool init_static_tls(const MemoryModule& memoryModule)
	{
		return detail::add_static_tls_entry(memoryModule);
	}
}
