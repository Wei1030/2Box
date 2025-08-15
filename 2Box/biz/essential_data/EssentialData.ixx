module;
#include "res/resource.h"
export module EssentialData;

import "sys_defs.h";
import std;
import Scheduler;
import MainApp;
import PELoader;
import SymbolLoader;
import Injector;
import Utility.SystemInfo;

namespace biz
{
	namespace detail
	{
		template <ArchBit BitType = CURRENT_ARCH_BIT>
		const char* get_dll_resource()
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

		template <ArchBit BitType = CURRENT_ARCH_BIT>
		void init_kernel32_info(Kernel32DllInfo& info)
		{
			const sys_info::SysDllMapHelper mapped = sys_info::get_kernel32_mapped<BitType>();
			pe::Parser<pe::parser_flag::HasSectionAligned> parser{static_cast<const char*>(mapped.memAddress())};
			info.rvaLoadLibraryA = parser.getProcRVA("LoadLibraryA");
			info.rvaGetProcAddress = parser.getProcRVA("GetProcAddress");
			info.rvaFlushInstructionCache = parser.getProcRVA("FlushInstructionCache");
		}
	}

	export
	template <ArchBit BitType = CURRENT_ARCH_BIT>
	pe::MemoryModule& get_memory_module()
	{
		static pe::MemoryModule memModule{pe::Parser<>{detail::get_dll_resource<BitType>()}};
		return memModule;
	}

	export
	template <ArchBit BitType = CURRENT_ARCH_BIT>
	InjectionDllsT<BitType> get_injection_dlls()
	{
		if constexpr (BitType == ArchBit::Bit64)
		{
			return {biz::get_memory_module<ArchBit::Bit32>(), biz::get_memory_module<ArchBit::Bit64>()};
		}
		else
		{
			return {biz::get_memory_module<ArchBit::Bit32>()};
		}
	}

	export std::uint64_t get_random_number()
	{
		thread_local std::mt19937_64 rng{std::random_device{}()};
		std::uniform_int_distribution<std::uint64_t> dis;
		return dis(rng);
	}

	export EssentialDataWrapper& get_essential_data()
	{
		struct InitializeOnceHelper
		{
			InitializeOnceHelper()
			{
				detail::init_os_version(wrapper.data.version);
				detail::init_kernel32_info<ArchBit::Bit32>(wrapper.data.kernelInfo32);
				if constexpr (IS_CURRENT_ARCH_64_BIT)
				{
					detail::init_kernel32_info<ArchBit::Bit64>(wrapper.data.kernelInfo64);
				}
				namespace fs = std::filesystem;
				wrapper.envDir = fs::path{fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"})}.native();
			}

			EssentialDataWrapper wrapper;
		};
		static InitializeOnceHelper dataInitialized;
		return dataInitialized.wrapper;
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

	export
	template <ArchBit BitType = CURRENT_ARCH_BIT>
	void init_symbols()
	{
		try
		{
			const symbols::Loader loader{symbols::default_symbols_path()};
			EssentialData& data = get_essential_data().data;
			const NtdllSymbolRvaInfo rvaInfo = detail::init_symbols(loader.loadNtdllSymbol<BitType>());
			if constexpr (BitType == ArchBit::Bit32)
			{
				data.symRva32 = rvaInfo;
				pe::g_sym_rva32 = rvaInfo;
			}
			else
			{
				data.symRva64 = rvaInfo;
				pe::g_sym_rva64 = rvaInfo;
			}
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
}
