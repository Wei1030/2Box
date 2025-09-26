module;
#include "res/resource.h"
export module EssentialData;

import "sys_defs.h";
import std;

namespace biz
{
	export struct DllResourceInfo
	{
		char* address;
		unsigned int size;
	};

	export struct CoreData
	{
		SystemVersionInfo version;
		DllResourceInfo dll32;
		DllResourceInfo dll64;
	};

	inline CoreData g_core_data;

	namespace detail
	{
		template <ArchBit BitType = CURRENT_ARCH_BIT>
		DllResourceInfo get_dll_resource()
		{
			HRSRC hRes;
			if constexpr (BitType == ArchBit::Bit32)
			{
				hRes = FindResourceW(nullptr, MAKEINTRESOURCEW(IDR_MEM_DLL_32), RT_RCDATA);
			}
			else
			{
				hRes = FindResourceW(nullptr, MAKEINTRESOURCEW(IDR_MEM_DLL_64), RT_RCDATA);
			}
			if (!hRes)
			{
				throw std::runtime_error("failed to find resource");
			}

			const HGLOBAL hResourceLoaded = LoadResource(nullptr, hRes);
			if (!hResourceLoaded)
			{
				throw std::runtime_error("failed to load resource");
			}

			return {
				static_cast<char*>(LockResource(hResourceLoaded)),
				SizeofResource(nullptr, hRes)
			};
		}

		// void init_os_version(SystemVersionInfo& version)
		// {
		// 	version.isWindows8Point1OrGreater = IsWindows8Point1OrGreater();
		// 	version.isWindows8OrGreater = IsWindows8OrGreater();
		// 	version.isWindowsVistaOrGreater = IsWindowsVistaOrGreater();
		//
		// 	SYSTEM_INFO sysInfo{};
		// 	GetNativeSystemInfo(&sysInfo);
		// 	version.is32BitSystem = sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM;
		// 	pe::g_os_version = version;
		// }

		// template <ArchBit BitType = CURRENT_ARCH_BIT>
		// void init_kernel32_info(Kernel32DllInfo& info)
		// {
		// 	const sys_info::SysDllMapHelper mapped = sys_info::get_kernel32_mapped<BitType>();
		// 	pe::Parser<pe::parser_flag::HasSectionAligned> parser{static_cast<const char*>(mapped.memAddress())};
		// 	info.rvaLoadLibraryA = parser.getProcRVA("LoadLibraryA");
		// 	info.rvaGetProcAddress = parser.getProcRVA("GetProcAddress");
		// 	info.rvaFlushInstructionCache = parser.getProcRVA("FlushInstructionCache");
		// }
	}

	export void init_system_version_info()
	{
		SystemVersionInfo& version = g_core_data.version;
		version.isWindows8Point1OrGreater = IsWindows8Point1OrGreater();
		version.isWindows8OrGreater = IsWindows8OrGreater();
		version.isWindowsVistaOrGreater = IsWindowsVistaOrGreater();

		SYSTEM_INFO sysInfo{};
		GetNativeSystemInfo(&sysInfo);
		version.is32BitSystem = sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM;
	}

	export void init_resource_info()
	{
		g_core_data.dll32 = detail::get_dll_resource<ArchBit::Bit32>();
		g_core_data.dll64 = detail::get_dll_resource<ArchBit::Bit64>();
	}

	export const CoreData& get_core_data() noexcept { return g_core_data; }

	// export
	// template <ArchBit BitType = CURRENT_ARCH_BIT>
	// pe::MemoryModule& get_memory_module()
	// {
	// 	static pe::MemoryModule memModule{pe::Parser<>{get_dll_resource_inst<BitType>().address}};
	// 	return memModule;
	// }

	// export
	// template <ArchBit BitType = CURRENT_ARCH_BIT>
	// InjectionDllsT<BitType> get_injection_dlls()
	// {
	// 	if constexpr (BitType == ArchBit::Bit64)
	// 	{
	// 		return {biz::get_memory_module<ArchBit::Bit32>(), biz::get_memory_module<ArchBit::Bit64>()};
	// 	}
	// 	else
	// 	{
	// 		return {biz::get_memory_module<ArchBit::Bit32>()};
	// 	}
	// }

	// export EssentialData& get_essential_data()
	// {
	// 	struct EssentialDataWrapper
	// 	{
	// 		EssentialDataWrapper()
	// 		{
	// 			detail::init_os_version(data.version);
	// 			detail::init_kernel32_info<ArchBit::Bit32>(data.kernelInfo32);
	// 			if constexpr (IS_CURRENT_ARCH_64_BIT)
	// 			{
	// 				detail::init_kernel32_info<ArchBit::Bit64>(data.kernelInfo64);
	// 			}
	// 		}
	//
	// 		EssentialData data;
	// 	};
	// 	static EssentialDataWrapper wrapper;
	// 	return wrapper.data;
	// }

	namespace detail
	{
		// [[nodiscard]] NtdllSymbolRvaInfo init_symbols(const symbols::Symbol& symbol)
		// {
		// 	NtdllSymbolRvaInfo result{};
		// 	result.LdrpHandleTlsData = symbol.symRvaFromName(L"LdrpHandleTlsData");
		// 	if (!result.LdrpHandleTlsData)
		// 	{
		// 		throw std::runtime_error{"LdrpHandleTlsData not found"};
		// 	}
		//
		// 	result.RtlInsertInvertedFunctionTable = symbol.symRvaFromName(L"RtlInsertInvertedFunctionTable");
		// 	if (!result.RtlInsertInvertedFunctionTable)
		// 	{
		// 		throw std::runtime_error{"RtlInsertInvertedFunctionTable not found"};
		// 	}
		//
		// 	// ReSharper disable once GrammarMistakeInComment
		// 	// result.LdrpReleaseTlsEntry = symbol.symRvaFromName(L"LdrpReleaseTlsEntry");
		// 	// if (!result.LdrpReleaseTlsEntry)
		// 	// {
		// 	// 	throw std::runtime_error{": LdrpReleaseTlsEntry not found"};
		// 	// }
		//
		// 	result.LdrpInvertedFunctionTable = symbol.symRvaFromName(L"LdrpInvertedFunctionTable");
		// 	if (!result.LdrpInvertedFunctionTable)
		// 	{
		// 		// win11
		// 		result.LdrpInvertedFunctionTable = symbol.symRvaFromName(L"LdrpInvertedFunctionTables");
		// 		if (!result.LdrpInvertedFunctionTable)
		// 		{
		// 			if (!IsWindows8OrGreater())
		// 			{
		// 				throw std::runtime_error{"LdrpInvertedFunctionTables not found"};
		// 			}
		// 		}
		// 	}
		// 	return result;
		// }
	}

	// export
	// template <ArchBit BitType = CURRENT_ARCH_BIT>
	// void init_symbols()
	// {
	// 	try
	// 	{
	// 		const symbols::Loader loader{symbols::default_symbols_path()};
	// 		EssentialData& data = get_essential_data();
	// 		const NtdllSymbolRvaInfo rvaInfo = detail::init_symbols(loader.loadNtdllSymbol<BitType>());
	// 		if constexpr (BitType == ArchBit::Bit32)
	// 		{
	// 			data.symRva32 = rvaInfo;
	// 			pe::g_sym_rva32 = rvaInfo;
	// 		}
	// 		else
	// 		{
	// 			data.symRva64 = rvaInfo;
	// 			pe::g_sym_rva64 = rvaInfo;
	// 		}
	// 	}
	// 	catch (const std::exception& e)
	// 	{
	// 		app().get_scheduler().addTask([errorMsg = std::string{e.what()}]
	// 		{
	// 			// ReSharper disable once StringLiteralTypo
	// 			throw std::runtime_error{std::format("The current system file analysis has failed, 2Box cannot run properly\nerror msg: {}", errorMsg)};
	// 		});
	// 	}
	// 	catch (...)
	// 	{
	// 		app().get_scheduler().addTask([]
	// 		{
	// 			// ReSharper disable once StringLiteralTypo
	// 			throw std::runtime_error("The current system file analysis has failed, 2Box cannot run properly\nerror msg: Unknown error");
	// 		});
	// 	}
	// }
}
