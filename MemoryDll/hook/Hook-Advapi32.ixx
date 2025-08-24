export module Hook:Advapi32;

import "sys_defs.h";
import std;
import :Core;
import GlobalData;
import PELoader;

namespace hook
{
	LSTATUS APIENTRY RegLoadAppKeyW(_In_ LPCWSTR lpFile, _Out_ PHKEY phkResult, _In_ REGSAM samDesired, _In_ DWORD dwOptions, _Reserved_ DWORD Reserved)
	{
		namespace fs = std::filesystem;
		return RegCreateKeyExW(global::Data::get().appKey(),
		                       std::format(L"{}_{}", fs::path{lpFile}.stem().native(), global::Data::get().envFlagName()).c_str(),
		                       Reserved, nullptr, dwOptions, samDesired, nullptr, phkResult, nullptr);
	}

	BOOL WINAPI CreateProcessAsUserA(_In_opt_ HANDLE hToken, _In_opt_ LPCSTR lpApplicationName, _Inout_opt_ LPSTR lpCommandLine,
	                                 _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes, _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	                                 _In_ BOOL bInheritHandles, _In_ DWORD dwCreationFlags, _In_opt_ LPVOID lpEnvironment,
	                                 _In_opt_ LPCSTR lpCurrentDirectory, _In_ LPSTARTUPINFOA lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation)
	{
		return ::CreateProcessA(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
		                        dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	BOOL WINAPI CreateProcessAsUserW(_In_opt_ HANDLE hToken, _In_opt_ LPCWSTR lpApplicationName, _Inout_opt_ LPWSTR lpCommandLine,
	                                 _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes, _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes, _In_ BOOL bInheritHandles,
	                                 _In_ DWORD dwCreationFlags, _In_opt_ LPVOID lpEnvironment, _In_opt_ LPCWSTR lpCurrentDirectory,
	                                 _In_ LPSTARTUPINFOW lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation)
	{
		return ::CreateProcessW(lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
		                        dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	BOOL WINAPI CreateProcessWithLogonW(_In_ LPCWSTR lpUsername, _In_opt_ LPCWSTR lpDomain, _In_ LPCWSTR lpPassword,
	                                    _In_ DWORD dwLogonFlags, _In_opt_ LPCWSTR lpApplicationName, _Inout_opt_ LPWSTR lpCommandLine,
	                                    _In_ DWORD dwCreationFlags, _In_opt_ LPVOID lpEnvironment, _In_opt_ LPCWSTR lpCurrentDirectory,
	                                    _In_ LPSTARTUPINFOW lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation)
	{
		return ::CreateProcessW(lpApplicationName, lpCommandLine, nullptr, nullptr, FALSE,
		                        dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	BOOL WINAPI CreateProcessWithTokenW(_In_ HANDLE hToken, _In_ DWORD dwLogonFlags,
	                                    _In_opt_ LPCWSTR lpApplicationName, _Inout_opt_ LPWSTR lpCommandLine, _In_ DWORD dwCreationFlags,
	                                    _In_opt_ LPVOID lpEnvironment, _In_opt_ LPCWSTR lpCurrentDirectory,
	                                    _In_ LPSTARTUPINFOW lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation)
	{
		return ::CreateProcessW(lpApplicationName, lpCommandLine, nullptr, nullptr, FALSE,
		                        dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	}

	void hook_advapi32()
	{
		// win7 or earlier
		if (!pe::g_os_version.isWindows8OrGreater)
		{
			create_hook_by_func_ptr<&::RegLoadAppKeyW>().setHook(&RegLoadAppKeyW);
		}

		create_hook_by_func_ptr<&::CreateProcessAsUserA>().setHook(&CreateProcessAsUserA);
		create_hook_by_func_ptr<&::CreateProcessAsUserW>().setHook(&CreateProcessAsUserW);
		create_hook_by_func_ptr<&::CreateProcessWithLogonW>().setHook(&CreateProcessWithLogonW);
		create_hook_by_func_ptr<&::CreateProcessWithTokenW>().setHook(&CreateProcessWithTokenW);
	}
}
