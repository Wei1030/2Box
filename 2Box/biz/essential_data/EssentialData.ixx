module;
#include "res/resource.h"
export module EssentialData;

import "sys_defs.h";
import std;
import Scheduler;
import MainApp;
import PELoader;
import SymbolLoader;
import Utility.SystemInfo;

namespace biz
{
	namespace detail
	{
		template<bool Is32Bit>
		const char* get_dll_resource()
		{
			HRSRC hRes;
			if constexpr (Is32Bit)
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
			return static_cast<const char*>(LockResource(hResourceLoaded));
		}
		
		void init_os_version(SystemVersionInfo& version)
		{
			version.isWindows8Point1OrGreater = IsWindows8Point1OrGreater();
			version.isWindows8OrGreater = IsWindows8OrGreater();
			version.isWindowsVistaOrGreater = IsWindowsVistaOrGreater();

			SYSTEM_INFO sysInfo{};
			GetNativeSystemInfo(&sysInfo);
			version.is32BitSystem = sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL || sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM;
			pe::g_os_version = version;
		}
		
		void init_kernel32_info32(Kernel32DllInfo& info)
		{
			const sys_info::SysDllMapHelper mapped = sys_info::get_kernel32_mapped32();
			pe::Parser<pe::parser_flag::HasSectionAligned> parser{static_cast<const char*>(mapped.memAddress())};
			info.rvaLoadLibraryA = parser.getProcRVA("LoadLibraryA");
			info.rvaGetProcAddress = parser.getProcRVA("GetProcAddress");
			info.rvaFlushInstructionCache = parser.getProcRVA("FlushInstructionCache");
		}
		
#ifdef _WIN64
		void init_kernel32_info64(Kernel32DllInfo& info)
		{
			const sys_info::SysDllMapHelper mapped = sys_info::get_kernel32_mapped64();
			pe::Parser<pe::parser_flag::HasSectionAligned> parser{static_cast<const char*>(mapped.memAddress())};
			info.rvaLoadLibraryA = parser.getProcRVA("LoadLibraryA");
			info.rvaGetProcAddress = parser.getProcRVA("GetProcAddress");
			info.rvaFlushInstructionCache = parser.getProcRVA("FlushInstructionCache");
		}
#endif
	}

	export
	template<bool Is32Bit>
	pe::MemoryModule& get_memory_module()
	{
		static pe::MemoryModule memModule{pe::Parser<>{detail::get_dll_resource<Is32Bit>()}};
		return memModule;
	}

	export EssentialData& get_essential_data()
	{
		struct EssentialDataWrapper
		{
			EssentialDataWrapper()
			{
				detail::init_os_version(data.version);
				detail::init_kernel32_info32(data.kernelInfo32);
#ifdef _WIN64
				detail::init_kernel32_info64(data.kernelInfo64);
#endif
			}

			EssentialData data;
		};
		static EssentialDataWrapper wrapper;
		return wrapper.data;
	}

	namespace detail
	{
		[[nodiscard]] NtdllSymbolRvaInfo init_symbols(const symbols::Symbol& symbol)
		{
			NtdllSymbolRvaInfo result{};
			result.LdrpHandleTlsData = symbol.symRvaFromName(L"LdrpHandleTlsData");
			if (!result.LdrpHandleTlsData)
			{
				throw std::runtime_error{"LdrpHandleTlsData not found"};
			}

			result.RtlInsertInvertedFunctionTable = symbol.symRvaFromName(L"RtlInsertInvertedFunctionTable");
			if (!result.RtlInsertInvertedFunctionTable)
			{
				throw std::runtime_error{"RtlInsertInvertedFunctionTable not found"};
			}

			// ReSharper disable once GrammarMistakeInComment
			// result.LdrpReleaseTlsEntry = symbol.symRvaFromName(L"LdrpReleaseTlsEntry");
			// if (!result.LdrpReleaseTlsEntry)
			// {
			// 	throw std::runtime_error{": LdrpReleaseTlsEntry not found"};
			// }

			result.LdrpInvertedFunctionTable = symbol.symRvaFromName(L"LdrpInvertedFunctionTable");
			if (!result.LdrpInvertedFunctionTable)
			{
				// win11
				result.LdrpInvertedFunctionTable = symbol.symRvaFromName(L"LdrpInvertedFunctionTables");
				if (!result.LdrpInvertedFunctionTable)
				{
					if (!IsWindows8OrGreater())
					{
						throw std::runtime_error{"LdrpInvertedFunctionTables not found"};
					}
				}
			}
			return result;
		}
	}

	export void init_symbols32()
	{
		try
		{
			const symbols::Loader loader{symbols::default_symbols_path()};
			EssentialData& data = get_essential_data();
			data.symRva32 = detail::init_symbols(loader.loadNtdllSymbol32());
			pe::g_sym_rva32 = data.symRva32;
		}
		catch (const std::exception& e)
		{
			app().get_scheduler().addTask([errorMsg = std::string{e.what()}]
			{
				// ReSharper disable once StringLiteralTypo
				throw std::runtime_error{std::format("The current system file analysis has failed, 2Box cannot run properly\nerror msg: {}", errorMsg)};
			});
		}
		catch (...)
		{
			app().get_scheduler().addTask([]
			{
				// ReSharper disable once StringLiteralTypo
				throw std::runtime_error("The current system file analysis has failed, 2Box cannot run properly\nerror msg: Unknown error");
			});
		}
	}

#ifdef _WIN64
	export void init_symbols64()
	{
		try
		{
			const symbols::Loader loader{symbols::default_symbols_path()};
			EssentialData& data = get_essential_data();
			data.symRva64 = detail::init_symbols(loader.loadNtdllSymbol64());
			pe::g_sym_rva64 = data.symRva64;
		}
		catch (const std::exception& e)
		{
			app().get_scheduler().addTask([errorMsg = std::string{e.what()}]
			{
				// ReSharper disable once StringLiteralTypo
				throw std::runtime_error{std::format("The current system file analysis has failed, 2Box cannot run properly\nerror msg: {}", errorMsg)};
			});
		}
		catch (...)
		{
			app().get_scheduler().addTask([]
			{
				// ReSharper disable once StringLiteralTypo
				throw std::runtime_error("The current system file analysis has failed, 2Box cannot run properly\nerror msg: Unknown error");
			});
		}
	}
#endif
}
