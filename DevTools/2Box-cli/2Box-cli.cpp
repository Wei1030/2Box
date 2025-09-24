// ReSharper disable once CppUnusedIncludeDirective
// ReSharper disable CppClangTidyClangDiagnosticUnusedMemberFunction
// ReSharper disable CppClangTidyClangDiagnosticUnusedFunction
// ReSharper disable CppClangTidyClangDiagnosticLanguageExtensionToken
// ReSharper disable CppClangTidyBugproneEmptyCatch
#include <winsdkver.h>

// ReSharper disable once IdentifierTypo
#define WINVER			0x0601
// ReSharper disable once CppInconsistentNaming
#define _WIN32_WINNT	0x0601  // NOLINT(clang-diagnostic-reserved-macro-identifier)

#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
// Windows 头文件
#include <windows.h>
#include <shellapi.h>

#include <cstdlib>
#include <string_view>
#include <utility>
#include <format>
#include <filesystem>
#include "service_h.h"

// midl要求实现的函数:
extern "C" {
void* __RPC_USER midl_user_allocate(size_t size)
{
	return malloc(size);
}

void __RPC_USER midl_user_free(void* ptr)
{
	free(ptr);
}
}

namespace
{
	class ClientBindingString
	{
	public:
		ClientBindingString() = default;

		~ClientBindingString()
		{
			if (m_pszStringBinding)
			{
				RpcStringFreeA(&m_pszStringBinding);
			}
		}

		ClientBindingString(const ClientBindingString&) = delete;
		ClientBindingString& operator=(const ClientBindingString&) = delete;

		ClientBindingString(ClientBindingString&& that) noexcept : m_pszStringBinding(std::exchange(that.m_pszStringBinding, nullptr))
		{
		}

		ClientBindingString& operator=(ClientBindingString&& that) noexcept
		{
			std::swap(m_pszStringBinding, that.m_pszStringBinding);
			return *this;
		}

		operator unsigned char*() const
		{
			return m_pszStringBinding;
		}

	protected:
		unsigned char* m_pszStringBinding{nullptr};
	};

	class ClientAlpcBindingString : public ClientBindingString
	{
	public:
		explicit ClientAlpcBindingString(std::string_view endPoint)
		{
			RPC_STATUS status = RpcStringBindingComposeA(nullptr, RPC_CSTR("ncalrpc"), nullptr, RPC_CSTR(endPoint.data()), nullptr, &m_pszStringBinding);
			if (status != RPC_S_OK)
			{
				throw std::runtime_error(std::format("RpcStringBindingComposeA failed, return status: {}", status));
			}
		}
	};

	class ClientBindingHandle
	{
	public:
		ClientBindingHandle() = default;

		explicit ClientBindingHandle(const ClientBindingString& bindingString)
		{
			RPC_STATUS status = RpcBindingFromStringBindingA(bindingString, &m_hBindingHandle);
			if (status != RPC_S_OK)
			{
				throw std::runtime_error(std::format("RpcBindingFromStringBindingA failed, return status: {}", status));
			}
		}

		ClientBindingHandle(const ClientBindingHandle& rhs)
		{
			if (rhs.m_hBindingHandle)
			{
				RPC_STATUS status = RpcBindingCopy(rhs.m_hBindingHandle, &m_hBindingHandle);
				if (status != RPC_S_OK)
				{
					throw std::runtime_error(std::format("RpcBindingCopy failed, return status: {}", status));
				}
			}
		}

		ClientBindingHandle(ClientBindingHandle&& that) noexcept : m_hBindingHandle(std::exchange(that.m_hBindingHandle, nullptr))
		{
		}

		~ClientBindingHandle()
		{
			if (m_hBindingHandle)
			{
				RpcBindingFree(&m_hBindingHandle);
			}
		}

		void swap(ClientBindingHandle& rhs) noexcept
		{
			const handle_t tmp = m_hBindingHandle;
			m_hBindingHandle = rhs.m_hBindingHandle;
			rhs.m_hBindingHandle = tmp;
		}

		friend void swap(ClientBindingHandle& c1, ClientBindingHandle& c2) noexcept
		{
			c1.swap(c2);
		}

		ClientBindingHandle& operator=(const ClientBindingHandle& rhs)
		{
			ClientBindingHandle(rhs).swap(*this);
			return *this;
		}

		ClientBindingHandle& operator=(ClientBindingHandle&& that) noexcept
		{
			std::swap(m_hBindingHandle, that.m_hBindingHandle);
			return *this;
		}

		handle_t getHandle() const
		{
			return m_hBindingHandle;
		}

		constexpr explicit operator bool() const noexcept
		{
			return m_hBindingHandle != nullptr;
		}

	private:
		handle_t m_hBindingHandle{nullptr};
	};

	const ClientBindingHandle& get_default_binding_info()
	{
		static ClientBindingHandle defaultBindingInfo(ClientAlpcBindingString{"{63B40BDA-A2D1-4516-BDBB-E1E2A960D31E}2BoxServer"});
		return defaultBindingInfo;
	}

	class ClientDefault
	{
	public:
		ClientDefault() : m_handle(get_default_binding_info().getHandle())
		{
		}

		handle_t getHandle() const
		{
			return m_handle;
		}

		decltype(auto) createProcess(const wchar_t appPath[], const wchar_t params[]) const
		{
			return create_process(m_handle, appPath, params);
		}

	private:
		handle_t m_handle;
	};

	int filter_offline_error(unsigned int code)
	{
		if (RpcExceptionFilter(code) == EXCEPTION_CONTINUE_SEARCH)
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}
		if (code == 0xE06D7363)
		{
			return EXCEPTION_CONTINUE_SEARCH;
		}
		return EXCEPTION_EXECUTE_HANDLER;
	}

	bool cli_create_process(const std::wstring& path, const std::wstring& params)
	{
		__try
		{
			const ClientDefault c;
			c.createProcess(path.c_str(), params.c_str());
			return true;
		}
		__except (filter_offline_error(RpcExceptionCode()))
		{
			return false;
		}
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	try
	{
		if (!lpCmdLine || wcslen(lpCmdLine) == 0)
		{
			return -3;
		}

		int numArgs;
		LPWSTR* cmdArray = CommandLineToArgvW(lpCmdLine, &numArgs);
		if (!cmdArray)
		{
			return static_cast<int>(GetLastError());
		}
		if (numArgs == 0)
		{
			return -4;
		}
		std::wstring app = cmdArray[0];
		std::wstring params = numArgs >= 2 ? cmdArray[1] : L"";
		LocalFree(cmdArray);

		if (cli_create_process(app, params))
		{
			return 0;
		}
		constexpr DWORD pathLength = std::numeric_limits<short>::max();
		std::wstring selfFullName;
		selfFullName.resize(pathLength);
		const DWORD resultSize = GetModuleFileNameW(nullptr, selfFullName.data(), pathLength);
		selfFullName.resize(resultSize);
		selfFullName = std::wstring(selfFullName);

		namespace fs = std::filesystem;
		const fs::path fsPath = fs::absolute(fs::path(selfFullName));
		std::wstring selfDir = fsPath.parent_path().native();

		PROCESS_INFORMATION procInfo = {nullptr};
		STARTUPINFOW startupInfo = {sizeof(startupInfo)};
		startupInfo.dwFlags = STARTF_USESHOWWINDOW;
		startupInfo.wShowWindow = SW_HIDE;

		if (!CreateProcessW(fs::weakly_canonical(fs::path{selfDir} / fs::path{L"2Box.exe"}).native().c_str(),
		                    nullptr, nullptr, nullptr,
		                    FALSE, CREATE_DEFAULT_ERROR_MODE, nullptr,
		                    selfDir.c_str(), &startupInfo, &procInfo))
		{
			return static_cast<int>(GetLastError());
		}
		WaitForInputIdle(procInfo.hProcess, 15000);
		CloseHandle(procInfo.hThread);
		CloseHandle(procInfo.hProcess);
		return cli_create_process(app, params) ? 0 : -2;
	}
	catch (...)
	{
	}
	return -1;
}
