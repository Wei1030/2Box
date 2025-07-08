// ReSharper disable CppZeroConstantCanBeReplacedWithNullptr
export module WinHttp;

import std;
import "sys_defs.h";
import Coroutine;

namespace ms
{
	class WinHttpSession;
	class WinHttpConnection;
	class WinHttpRequest;

	class InternetHandle
	{
	public:
		InternetHandle(HINTERNET handle) noexcept : m_handle(handle)
		{
		}

		operator HINTERNET() const { return m_handle; }

		~InternetHandle()
		{
			if (m_handle)
			{
				WinHttpCloseHandle(m_handle);
			}
		}

		InternetHandle(const InternetHandle&) = delete;
		InternetHandle& operator=(const InternetHandle&) = delete;

		InternetHandle(InternetHandle&& that) noexcept : m_handle(std::exchange(that.m_handle, nullptr))
		{
		}

		InternetHandle& operator=(InternetHandle&& that) noexcept
		{
			std::swap(m_handle, that.m_handle);
			return *this;
		}

		explicit operator bool() const noexcept
		{
			return m_handle != nullptr;
		}

	private:
		HINTERNET m_handle{nullptr};
	};

	class WinHttpSession : std::enable_shared_from_this<WinHttpSession>
	{
	public:
		WinHttpSession()
		{
			m_hSession = WinHttpOpen(L"WinHttp", WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
			if (!m_hSession)
			{
				throw std::runtime_error(std::format("WinHttpOpen failed, error code: {}", GetLastError()));
			}

			DWORD dwFlags = WINHTTP_FLAG_SECURE_PROTOCOL_SSL3
				| WINHTTP_FLAG_SECURE_PROTOCOL_TLS1
				| WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_1
				| WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2;
			WinHttpSetOption(m_hSession, WINHTTP_OPTION_SECURE_PROTOCOLS, &dwFlags, sizeof(dwFlags));
		}

		inline std::shared_ptr<WinHttpConnection> createConnection(std::wstring_view serverName, unsigned short serverPort = INTERNET_DEFAULT_PORT);

		HINTERNET handle() const
		{
			return m_hSession;
		}

	private:
		InternetHandle m_hSession{nullptr};
	};

	class WinHttpConnection : std::enable_shared_from_this<WinHttpConnection>
	{
	public:
		WinHttpConnection(std::shared_ptr<WinHttpSession> hSession, std::wstring_view serverName, unsigned short serverPort = INTERNET_DEFAULT_PORT)
		{
			m_hSession = std::move(hSession);
			m_hConnect = WinHttpConnect(m_hSession->handle(), serverName.data(), serverPort, 0);
			if (!m_hConnect)
			{
				throw std::runtime_error(std::format("WinHttpConnect failed, error code: {}", GetLastError()));
			}
		}

		inline std::shared_ptr<WinHttpRequest> openRequest(std::wstring_view verb, std::wstring_view objName,
		                                                   std::wstring_view version = {}, std::wstring_view referrer = {},
		                                                   std::span<std::wstring_view> acceptTypes = {},
		                                                   DWORD flags = WINHTTP_FLAG_REFRESH | WINHTTP_FLAG_SECURE);

		HINTERNET handle() const
		{
			return m_hConnect;
		}

	private:
		std::shared_ptr<WinHttpSession> m_hSession;
		InternetHandle m_hConnect{nullptr};
	};

	inline std::shared_ptr<WinHttpConnection> WinHttpSession::createConnection(std::wstring_view serverName, unsigned short serverPort)
	{
		return std::make_shared<WinHttpConnection>(shared_from_this(), serverName, serverPort);
	}

	export std::shared_ptr<WinHttpSession> create_win_http_session()
	{
		return std::make_shared<WinHttpSession>();
	}

	class WinHttpRequest
	{
	public:
		WinHttpRequest(std::shared_ptr<WinHttpConnection> connection, std::wstring_view verb, std::wstring_view objName, std::wstring_view version, std::wstring_view referrer,
		               std::span<std::wstring_view> acceptTypes, DWORD flags)
			: m_connection(std::move(connection))
			  , m_verb(verb)
			  , m_objName(objName)
			  , m_version(version)
			  , m_referrer(referrer)
			  , m_flags(flags)
		{
			m_acceptTypes.reserve(acceptTypes.size());
			for (const auto& accept : acceptTypes)
			{
				m_acceptTypes.emplace_back(accept);
			}

			if (m_acceptTypes.size())
			{
				m_refAcceptTypes.reserve(m_acceptTypes.size() + 1);
				for (const auto& accept : m_acceptTypes)
				{
					m_refAcceptTypes.emplace_back(accept.c_str());
				}
				m_refAcceptTypes.emplace_back(nullptr);
			}
		}

		using RspContentLengthReporter = std::move_only_function<void(std::uint64_t)>;
		using RspProgressReporter = std::move_only_function<void(std::uint64_t)>;

		struct PerRequest
		{
			InternetHandle hRequest{nullptr};
			RspContentLengthReporter contentLengthReporter{};
			RspProgressReporter progressReporter{};
			std::atomic<bool> bSucceeded{false};
			DWORD errorCode{0};
			std::string errorMessage;
			coro::GuaranteedResolver<void> resolver;
			std::array<std::byte, 8192> buffer{};
			std::vector<std::byte> result{};
		};

		coro::LazyTask<std::vector<std::byte>> request(RspContentLengthReporter contentLengthReporter, RspProgressReporter progressReporter,
		                                               std::span<std::byte> optional = {}, std::wstring_view additionalHeaders = {})
		{
			PerRequest reqContext;
			reqContext.hRequest = WinHttpOpenRequest(m_connection->handle(),
			                                         m_verb.c_str(),
			                                         m_objName.c_str(),
			                                         m_version.empty() ? nullptr : m_version.c_str(),
			                                         m_referrer.empty() ? WINHTTP_NO_REFERER : m_referrer.c_str(),
			                                         m_refAcceptTypes.empty() ? WINHTTP_DEFAULT_ACCEPT_TYPES : m_refAcceptTypes.data(),
			                                         m_flags);
			if (!reqContext.hRequest)
			{
				throw std::runtime_error(std::format("WinHttpOpenRequest failed, error code: {}", GetLastError()));
			}

			std::stop_token token = co_await coro::get_current_cancellation_token();
			std::stop_callback stopCb{
				token, [&reqContext]()
				{
					reqContext.hRequest = nullptr;
				}
			};

			DWORD dwFlags = SECURITY_FLAG_IGNORE_ALL_CERT_ERRORS;
			WinHttpSetOption(reqContext.hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));

			WinHttpSetStatusCallback(reqContext.hRequest, &perRequestStatusCallback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);

			reqContext.contentLengthReporter = std::move(contentLengthReporter);
			reqContext.progressReporter = std::move(progressReporter);

			co_await coro::LazyTask<void>::create([&reqContext, &optional, &additionalHeaders, this](coro::GuaranteedResolver<void> resolver)
			{
				reqContext.resolver = std::move(resolver);
				// 异步的win http就算失败也会在回调中给失败的结果,这里不需要判断错误
				WinHttpSendRequest(reqContext.hRequest,
				                   additionalHeaders.empty() ? WINHTTP_NO_ADDITIONAL_HEADERS : additionalHeaders.data(),
				                   static_cast<DWORD>(additionalHeaders.size()),
				                   optional.empty() ? WINHTTP_NO_REQUEST_DATA : optional.data(),
				                   static_cast<DWORD>(optional.size()),
				                   static_cast<DWORD>(optional.size()),
				                   reinterpret_cast<DWORD_PTR>(&reqContext));
			});

			co_return std::move(reqContext.result);
		}

	private:
		static std::uint32_t queryContentLength(HINTERNET hRequest)
		{
			try
			{
				constexpr DWORD maxCount = 33;
				std::wstring contentLength(maxCount, 0);
				DWORD size = maxCount * 2;
				if (!WinHttpQueryHeaders(hRequest, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, contentLength.data(), &size, WINHTTP_NO_HEADER_INDEX))
				{
					return 0;
				}
				contentLength.resize(size);
				return std::stoul(contentLength);
			}
			catch (...)
			{
				return 0;
			}
		}

		static std::string_view mapAnyAsyncErrorToFunctionName(DWORD_PTR dwResult)
		{
			switch (dwResult)
			{
			case API_RECEIVE_RESPONSE:
				return "WinHttpReceiveResponse";
			case API_QUERY_DATA_AVAILABLE:
				return "WinHttpQueryDataAvailable";
			case API_READ_DATA:
				return "WinHttpReadData";
			case API_WRITE_DATA:
				return "WinHttpWriteData";
			case API_SEND_REQUEST:
				return "WinHttpSendRequest";
			case API_GET_PROXY_FOR_URL:
				return "WinHttpGetProxyForUrlEx";
			default:
				return "Unknown Function";
			}
		}

		static void perRequestStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
		{
			PerRequest* reqCtx = reinterpret_cast<PerRequest*>(dwContext);
			if (reqCtx->hRequest != hInternet)
			{
				return;
			}
			switch (dwInternetStatus)
			{
			case WINHTTP_CALLBACK_STATUS_SENDREQUEST_COMPLETE: // WinHttpSendRequest done
				if (!WinHttpReceiveResponse(hInternet, nullptr))
				{
					reqCtx->errorCode = GetLastError();
					reqCtx->errorMessage = std::format("WinHttpReceiveResponse failed, error code: {}", reqCtx->errorCode);
					reqCtx->bSucceeded.store(false, std::memory_order_release);
					reqCtx->hRequest = nullptr;
				}
				break;
			case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE: // WinHttpReceiveResponse done
				{
					const std::uint32_t contentLength = queryContentLength(hInternet);
					reqCtx->result.reserve(contentLength);
					if (reqCtx->contentLengthReporter)
					{
						reqCtx->contentLengthReporter(contentLength);
					}
					if (!WinHttpQueryDataAvailable(hInternet, nullptr))
					{
						reqCtx->errorCode = GetLastError();
						reqCtx->errorMessage = std::format("WinHttpQueryDataAvailable failed, error code: {}", reqCtx->errorCode);
						reqCtx->bSucceeded.store(false, std::memory_order_release);
						reqCtx->hRequest = nullptr;
					}
				}
				break;
			case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE: // WinHttpQueryDataAvailable done
				if (!WinHttpReadData(hInternet, reqCtx->buffer.data(), (DWORD)reqCtx->buffer.size(), nullptr))
				{
					reqCtx->errorCode = GetLastError();
					reqCtx->errorMessage = std::format("WinHttpReadData failed, error code: {}", reqCtx->errorCode);
					reqCtx->bSucceeded.store(false, std::memory_order_release);
					reqCtx->hRequest = nullptr;
				}
				break;
			case WINHTTP_CALLBACK_STATUS_READ_COMPLETE: // WinHttpReadData done
				if (reqCtx->progressReporter)
				{
					reqCtx->progressReporter(dwStatusInformationLength);
				}
				// 是否已经收完
				if (dwStatusInformationLength == 0)
				{
					reqCtx->bSucceeded.store(true, std::memory_order_release);
					reqCtx->hRequest = nullptr;
				}
				else
				{
					reqCtx->result.insert(reqCtx->result.end(), reqCtx->buffer.begin(), reqCtx->buffer.begin() + dwStatusInformationLength);
					if (!WinHttpReadData(hInternet, reqCtx->buffer.data(), (DWORD)reqCtx->buffer.size(), nullptr))
					{
						reqCtx->errorCode = GetLastError();
						reqCtx->errorMessage = std::format("WinHttpReadData failed, error code: {}", reqCtx->errorCode);
						reqCtx->bSucceeded.store(false, std::memory_order_release);
						reqCtx->hRequest = nullptr;
					}
				}
				break;
			case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR: // any error
				{
					WINHTTP_ASYNC_RESULT* res = static_cast<WINHTTP_ASYNC_RESULT*>(lpvStatusInformation);
					reqCtx->errorCode = reqCtx->errorCode;
					reqCtx->errorMessage = std::format("{} failed, error code: {}", mapAnyAsyncErrorToFunctionName(res->dwResult), reqCtx->errorCode);
					reqCtx->bSucceeded.store(false, std::memory_order_release);
					reqCtx->hRequest = nullptr;
				}
				break;
			case WINHTTP_CALLBACK_STATUS_HANDLE_CLOSING:
				if (reqCtx->bSucceeded.load(std::memory_order_acquire))
				{
					reqCtx->resolver->resolve();
				}
				else
				{
					if (reqCtx->errorMessage.size())
					{
						reqCtx->resolver->reject(std::make_exception_ptr(std::runtime_error(reqCtx->errorMessage)));
					}
					else
					{
						reqCtx->resolver->reject(std::make_exception_ptr(std::runtime_error("operation canceled")));
					}
				}
				break;
			default:
				break;
			}
		}

	private:
		std::shared_ptr<WinHttpConnection> m_connection;
		std::wstring m_verb;
		std::wstring m_objName;
		std::wstring m_version;
		std::wstring m_referrer;
		std::vector<std::wstring> m_acceptTypes;
		std::vector<const wchar_t*> m_refAcceptTypes;
		DWORD m_flags;
	};

	inline std::shared_ptr<WinHttpRequest> WinHttpConnection::openRequest(std::wstring_view verb, std::wstring_view objName, std::wstring_view version, std::wstring_view referrer,
	                                                                      std::span<std::wstring_view> acceptTypes, DWORD flags)
	{
		return std::make_shared<WinHttpRequest>(shared_from_this(), verb, objName, version, referrer, acceptTypes, flags);
	}
}
