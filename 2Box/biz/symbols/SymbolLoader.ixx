export module SymbolLoader;

import std;
import "sys_defs.h";

import Utility.SystemInfo;
import Coroutine;
import WinHttp;

namespace symbols
{
	export class Symbol
	{
	public:
		static SYMSRV_INDEX_INFOW fileIndexInfo(std::wstring_view filePath)
		{
			SYMSRV_INDEX_INFOW indexInfo{sizeof(SYMSRV_INDEX_INFOW)};
			if (!SymSrvGetFileIndexInfoW(filePath.data(), &indexInfo, 0))
			{
				throw std::runtime_error(std::format("SymSrvGetFileIndexInfoW failed, error code: {}", GetLastError()));
			}
			return indexInfo;
		}

		static void ensurePdbExists(const SYMSRV_INDEX_INFOW& indexInfo)
		{
			wchar_t pdbPath[MAX_PATH + 1] = {};
			if (!SymFindFileInPathW(GetCurrentProcess(), nullptr,
			                        indexInfo.pdbfile,
			                        const_cast<GUID*>(&indexInfo.guid),
			                        indexInfo.age,
			                        0,
			                        SSRVOPT_GUIDPTR,
			                        pdbPath,
			                        nullptr, nullptr))
			{
				throw std::runtime_error(std::format("SymFindFileInPathW failed, error code: {}", GetLastError()));
			}
		}

		explicit Symbol(std::wstring_view filePath)
		{
			ensurePdbExists(fileIndexInfo(filePath));

			m_modBase = SymLoadModuleExW(GetCurrentProcess(), nullptr, filePath.data(), nullptr, 0, 0, nullptr, 0);
			if (!m_modBase)
			{
				throw std::runtime_error(std::format("SymLoadModuleExW failed, error code: {}", GetLastError()));
			}
		}

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

	private:
		DWORD64 m_modBase{0};
	};

	export class Loader
	{
	public:
		explicit Loader(std::wstring_view searchPath) : m_searchPath(searchPath)
		{
			SymSetOptions(SYMOPT_EXACT_SYMBOLS | SYMOPT_UNDNAME);
			if (!SymInitializeW(GetCurrentProcess(), m_searchPath.c_str(), FALSE))
			{
				throw std::runtime_error(std::format("SymInitializeW failed, error code: {}", GetLastError()));
			}
			// createConnection实际不会进行网络连接, 只有开始请求时才会进行实际的连接
			m_connection = ms::get_default_win_http_session()->createConnection(L"msdl.microsoft.com");
		}

		~Loader() noexcept
		{
			SymCleanup(GetCurrentProcess());
		}

		Loader(const Loader&) = delete;
		Loader& operator=(const Loader&) = delete;
		Loader(Loader&&) = delete;
		Loader& operator=(Loader&&) = delete;

		using ContentLengthReporter = std::move_only_function<void(std::uint64_t)>;
		using ProgressReporter = std::move_only_function<void(std::uint64_t)>;

		coro::LazyTask<Symbol> loadNtdllSymbol(ContentLengthReporter contentLengthReporter, ProgressReporter progressReporter) const
		{
			namespace fs = std::filesystem;
			const fs::path systemDir{sys_info::get_system_dir()};
			const fs::path ntdllPath{fs::weakly_canonical(systemDir / fs::path{L"ntdll.dll"})};
			co_return co_await loadSymbol(ntdllPath.native(), std::move(contentLengthReporter), std::move(progressReporter));
		}

		coro::LazyTask<Symbol> loadWow64NtdllSymbol(ContentLengthReporter contentLengthReporter, ProgressReporter progressReporter) const
		{
			namespace fs = std::filesystem;
			const fs::path systemDir{sys_info::get_system_wow64_dir()};
			const fs::path ntdllPath{fs::weakly_canonical(systemDir / fs::path{L"ntdll.dll"})};
			co_return co_await loadSymbol(ntdllPath.native(), std::move(contentLengthReporter), std::move(progressReporter));
		}

		coro::LazyTask<Symbol> loadSymbol(std::wstring filePath, ContentLengthReporter contentLengthReporter, ProgressReporter progressReporter) const
		{
			namespace fs = std::filesystem;
			SYMSRV_INDEX_INFOW indexInfo = Symbol::fileIndexInfo(filePath);
			GUID& guid = indexInfo.guid;
			std::wstring strGuid = std::format(L"{:08X}{:04X}{:04X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:02X}{:X}", guid.Data1, guid.Data2, guid.Data3,
			                                   guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
			                                   guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7], indexInfo.age);
			const fs::path filePdbPath{fs::path{m_searchPath} / fs::path{indexInfo.pdbfile} / fs::path{strGuid} / fs::path{indexInfo.pdbfile}};
			if (!fs::exists(filePdbPath))
			{
				// start download
				std::shared_ptr<ms::WinHttpRequest> req = m_connection->openRequest(L"GET", std::format(L"download/symbols/{}/{}/{}", indexInfo.pdbfile, strGuid, indexInfo.pdbfile));
				std::vector<std::byte> bytes = co_await req->request(std::move(contentLengthReporter), std::move(progressReporter));
				// 将bytes写入文件...
				co_await writeFile(filePdbPath.native(), bytes);
			}
			co_return Symbol{filePath};
		}

	private:
		static void writeFileThread(std::wstring_view pdbFile, std::span<std::byte> bytes, const coro::GuaranteedResolver<void>& resolver)
		{
			namespace fs = std::filesystem;
			std::error_code ec;
			fs::create_directories(fs::path{pdbFile}.parent_path(), ec);
			if (ec)
			{
				resolver->reject(std::make_exception_ptr(std::runtime_error(std::format("create_directories failed, error code: {}", ec.value()))));
				return;
			}

			HANDLE handle = CreateFileW(std::format(L"\\\\?\\{}", pdbFile).c_str(),
			                            GENERIC_READ | GENERIC_WRITE,
			                            0,
			                            nullptr,
			                            CREATE_ALWAYS,
			                            FILE_ATTRIBUTE_NORMAL,
			                            nullptr);
			if (handle == INVALID_HANDLE_VALUE)
			{
				resolver->reject(std::make_exception_ptr(std::runtime_error(std::format("CreateFileW failed, error code: {}", GetLastError()))));
				return;
			}
			if (!WriteFile(handle, bytes.data(), static_cast<DWORD>(bytes.size()), nullptr, nullptr))
			{
				CloseHandle(handle);
				resolver->reject(std::make_exception_ptr(std::runtime_error(std::format("WriteFile failed, error code: {}", GetLastError()))));
				return;
			}
			CloseHandle(handle);
			resolver->resolve();
		}

		static coro::LazyTask<void> writeFile(std::wstring_view pdbFile, std::span<std::byte> bytes)
		{
			co_await coro::LazyTask<void>::create([pdbFile, bytes](coro::GuaranteedResolver<void> resolver)
			{
				// 反正设计成开始写文件后不允许取消
				// 直接简单的创建一个线程阻塞式写文件
				std::thread{
					[pdbFile, bytes, res = std::move(resolver)]
					{
						writeFileThread(pdbFile, bytes, res);
					}
				}.detach();
			});
			co_return;
		}

	private:
		std::wstring m_searchPath;
		std::shared_ptr<ms::WinHttpConnection> m_connection;
	};
}
