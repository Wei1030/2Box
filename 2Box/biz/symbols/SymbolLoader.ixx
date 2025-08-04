export module SymbolLoader;

import std;
import "sys_defs.h";

import MainApp;
import PELoader;
import Utility.SystemInfo;

namespace symbols
{
	export struct PdbInfo
	{
		std::wstring pdbDir;
		std::wstring pdbName;
		std::wstring downloadObjName;
	};

	export SYMSRV_INDEX_INFOW file_index_info(std::wstring_view filePath)
	{
		SYMSRV_INDEX_INFOW indexInfo{sizeof(SYMSRV_INDEX_INFOW)};
		if (!SymSrvGetFileIndexInfoW(filePath.data(), &indexInfo, 0))
		{
			throw std::runtime_error(std::format("SymSrvGetFileIndexInfoW failed, error code: {}", GetLastError()));
		}
		return indexInfo;
	}

	export PdbInfo parse_pdb_path(const SYMSRV_INDEX_INFOW& indexInfo, std::wstring_view rootPath = {})
	{
		namespace fs = std::filesystem;
		PdbInfo result;
		const GUID& guid = indexInfo.guid;
		const std::wstring strGuid = std::format(L"{:08X}{:04X}{:04X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:X}", guid.Data1, guid.Data2, guid.Data3,
		                                         guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		                                         guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], indexInfo.age);
		const fs::path filePdbDir{fs::path{rootPath} / fs::path{indexInfo.pdbfile} / fs::path{strGuid}};
		result.pdbDir = filePdbDir.native();
		result.pdbName = indexInfo.pdbfile;
		result.downloadObjName = std::format(L"download/symbols/{}/{}/{}", indexInfo.pdbfile, strGuid, indexInfo.pdbfile);
		return result;
	}

#ifdef _WIN64
	export PdbInfo get_ntdll64_pdb_info(std::wstring_view rootPath = {})
	{
		namespace fs = std::filesystem;
		const fs::path systemDir{sys_info::get_system_dir()};
		const fs::path ntdllPath{fs::weakly_canonical(systemDir / fs::path{L"ntdll.dll"})};
		const SYMSRV_INDEX_INFOW indexInfo = file_index_info(ntdllPath.native());
		return parse_pdb_path(indexInfo, rootPath);
	}
#endif

	export PdbInfo get_ntdll32_pdb_info(std::wstring_view rootPath = {})
	{
		namespace fs = std::filesystem;
#ifdef _WIN64
		const fs::path systemDir{sys_info::get_system_wow64_dir()};
#else
		const fs::path systemDir{sys_info::get_system_dir()};
#endif
		const fs::path ntdllPath{fs::weakly_canonical(systemDir / fs::path{L"ntdll.dll"})};
		const SYMSRV_INDEX_INFOW indexInfo = file_index_info(ntdllPath.native());
		return parse_pdb_path(indexInfo, rootPath);
	}

	export class Loader;

	export class Symbol
	{
	public:
		~Symbol()
		{
			if (m_modBase)
			{
				SymUnloadModule64(GetCurrentProcess(), m_modBase);
			}
		}

		Symbol(const Symbol&) = delete;
		Symbol& operator=(const Symbol&) = delete;

		Symbol(Symbol&& that) noexcept : m_modBase(std::exchange(that.m_modBase, 0))
		{
		}

		Symbol& operator=(Symbol&& that) noexcept
		{
			std::swap(m_modBase, that.m_modBase);
			return *this;
		}

		// ReSharper disable CppMemberFunctionMayBeStatic
		std::uint64_t symRvaFromName(std::wstring_view name) noexcept
		{
			SYMBOL_INFOW si{sizeof(SYMBOL_INFOW)};
			if (SymFromNameW(GetCurrentProcess(), name.data(), &si))
			{
				return si.Address - si.ModBase;
			}
			return 0;
		}

		// ReSharper restore CppMemberFunctionMayBeStatic

	private:
		friend class Loader;

		explicit Symbol(std::wstring_view filePath)
		{
			m_modBase = SymLoadModuleExW(GetCurrentProcess(), nullptr, filePath.data(), nullptr, 0, 0, nullptr, 0);
			if (!m_modBase)
			{
				throw std::runtime_error(std::format("SymLoadModuleExW failed, error code: {}", GetLastError()));
			}
		}

	private:
		DWORD64 m_modBase{0};
	};

	class Loader
	{
	public:
		explicit Loader(std::wstring_view searchPath) : m_searchPath(searchPath)
		{
			SymSetOptions(SYMOPT_EXACT_SYMBOLS | SYMOPT_UNDNAME);
			if (!SymInitializeW(GetCurrentProcess(), m_searchPath.c_str(), FALSE))
			{
				throw std::runtime_error(std::format("SymInitializeW failed, error code: {}", GetLastError()));
			}
		}

		~Loader() noexcept
		{
			SymCleanup(GetCurrentProcess());
		}

		Loader(const Loader&) = delete;
		Loader& operator=(const Loader&) = delete;
		Loader(Loader&&) = delete;
		Loader& operator=(Loader&&) = delete;

		// ReSharper disable CppMemberFunctionMayBeStatic
#ifdef _WIN64
		Symbol loadNtdllSymbol64() const
		{
			namespace fs = std::filesystem;
			const fs::path systemDir{sys_info::get_system_dir()};
			const fs::path ntdllPath{fs::weakly_canonical(systemDir / fs::path{L"ntdll.dll"})};
			return Symbol{ntdllPath.native()};
		}
#endif

		Symbol loadNtdllSymbol32() const
		{
			namespace fs = std::filesystem;
#ifdef _WIN64
			const fs::path systemDir{sys_info::get_system_wow64_dir()};
#else
			const fs::path systemDir{sys_info::get_system_dir()};
#endif
			const fs::path ntdllPath{fs::weakly_canonical(systemDir / fs::path{L"ntdll.dll"})};
			return Symbol{ntdllPath.native()};
		}

		// ReSharper restore CppMemberFunctionMayBeStatic

	private:
		std::wstring m_searchPath;
	};

	export void init_all_symbols32_for_pe_loader()
	{
		try
		{
			static std::wstring searchPath{std::format(L"{}\\Symbols", app().exeDir())};
			const Loader loader{searchPath};
			Symbol symbol32 = loader.loadNtdllSymbol32();
			pe::g_sym_rva.LdrpHandleTlsData32 = symbol32.symRvaFromName(L"LdrpHandleTlsData");
			if (!pe::g_sym_rva.LdrpHandleTlsData32)
			{
				throw std::runtime_error{"32: LdrpHandleTlsData not found"};
			}

			pe::g_sym_rva.RtlInsertInvertedFunctionTable32 = symbol32.symRvaFromName(L"RtlInsertInvertedFunctionTable");
			if (!pe::g_sym_rva.RtlInsertInvertedFunctionTable32)
			{
				throw std::runtime_error{"32: RtlInsertInvertedFunctionTable not found"};
			}

			// ReSharper disable once GrammarMistakeInComment
			// pe::g_sym_rva.LdrpReleaseTlsEntry32 = symbol32.symRvaFromName(L"LdrpReleaseTlsEntry");
			// if (!pe::g_sym_rva.LdrpReleaseTlsEntry32)
			// {
			// 	throw std::runtime_error{"32: LdrpReleaseTlsEntry not found"};
			// }

			pe::g_sym_rva.LdrpInvertedFunctionTable32 = symbol32.symRvaFromName(L"LdrpInvertedFunctionTable");
			if (!pe::g_sym_rva.LdrpInvertedFunctionTable32)
			{
				// win11
				pe::g_sym_rva.LdrpInvertedFunctionTable32 = symbol32.symRvaFromName(L"LdrpInvertedFunctionTables");
				if (!pe::g_sym_rva.LdrpInvertedFunctionTable32)
				{
					if (!IsWindows8OrGreater())
					{
						throw std::runtime_error{"32: LdrpInvertedFunctionTables not found"};
					}
				}
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

#ifdef _WIN64
	export void init_all_symbols64_for_pe_loader()
	{
		try
		{
			static std::wstring searchPath{std::format(L"{}\\Symbols", app().exeDir())};
			const Loader loader{searchPath};
			Symbol symbol64 = loader.loadNtdllSymbol64();
			pe::g_sym_rva.LdrpHandleTlsData64 = symbol64.symRvaFromName(L"LdrpHandleTlsData");
			if (!pe::g_sym_rva.LdrpHandleTlsData64)
			{
				throw std::runtime_error{"64: LdrpHandleTlsData not found"};
			}

			pe::g_sym_rva.RtlInsertInvertedFunctionTable64 = symbol64.symRvaFromName(L"RtlInsertInvertedFunctionTable");
			if (!pe::g_sym_rva.RtlInsertInvertedFunctionTable64)
			{
				throw std::runtime_error{"64: RtlInsertInvertedFunctionTable not found"};
			}

			// ReSharper disable once GrammarMistakeInComment
			// pe::g_sym_rva.LdrpReleaseTlsEntry64 = symbol64.symRvaFromName(L"LdrpReleaseTlsEntry");
			// if (!pe::g_sym_rva.LdrpReleaseTlsEntry64)
			// {
			// 	throw std::runtime_error{"64: LdrpReleaseTlsEntry not found"};
			// }

			pe::g_sym_rva.LdrpInvertedFunctionTable64 = symbol64.symRvaFromName(L"LdrpInvertedFunctionTable");
			if (!pe::g_sym_rva.LdrpInvertedFunctionTable64)
			{
				// win11
				pe::g_sym_rva.LdrpInvertedFunctionTable64 = symbol64.symRvaFromName(L"LdrpInvertedFunctionTables");
				if (!pe::g_sym_rva.LdrpInvertedFunctionTable64)
				{
					if (!IsWindows8OrGreater())
					{
						throw std::runtime_error{"64: LdrpInvertedFunctionTables not found"};
					}
				}
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
#endif
}
