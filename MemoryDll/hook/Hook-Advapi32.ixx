export module Hook:Advapi32;

import "sys_defs.h";
import std;
import :Core;
import GlobalData;
import :Kernel32;

namespace hook
{
	LSTATUS APIENTRY RegLoadAppKeyW(_In_ LPCWSTR lpFile, _Out_ PHKEY phkResult, _In_ REGSAM samDesired, _In_ DWORD dwOptions, _Reserved_ DWORD Reserved)
	{
		namespace fs = std::filesystem;
		return RegCreateKeyExW(global::Data::get().appKey(),
		                       std::format(L"{}_{}", fs::path{lpFile}.stem().native(), global::Data::get().envFlagName()).c_str(),
		                       Reserved, nullptr, dwOptions, samDesired, nullptr, phkResult, nullptr);
	}

	BOOL GetTokenIntegrityLevel(HANDLE hToken, PDWORD pIntegrityLevel)
	{
		BOOL bReturn = FALSE;

		// First, compute the size of the buffer to get the Integrity level
		DWORD dwNeededSize = 0;
		if (!GetTokenInformation(hToken, TokenIntegrityLevel, nullptr, 0, &dwNeededSize))
		{
			if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
			{
				// Second, allocate a memory block with the required size 
				if (const PTOKEN_MANDATORY_LABEL pTokenInfo = static_cast<PTOKEN_MANDATORY_LABEL>(LocalAlloc(0, dwNeededSize)); pTokenInfo)
				{
					// And finally, ask for the integrity level
					if (GetTokenInformation(hToken, TokenIntegrityLevel, pTokenInfo, dwNeededSize, &dwNeededSize))
					{
						*pIntegrityLevel = *GetSidSubAuthority(pTokenInfo->Label.Sid, *GetSidSubAuthorityCount(pTokenInfo->Label.Sid) - 1);
						bReturn = TRUE;
					}
					LocalFree(pTokenInfo);
				}
			}
		}
		return bReturn;
	}

	BOOL GetProcessIntegrityLevel(HANDLE hProcess, PDWORD pIntegrityLevel)
	{
		HANDLE hToken = nullptr;
		if (!OpenProcessToken(hProcess, TOKEN_READ, &hToken))
		{
			return FALSE;
		}
		const BOOL bReturn = GetTokenIntegrityLevel(hToken, pIntegrityLevel);
		CloseHandle(hToken);
		return bReturn;
	}

	template <auto Trampoline>
	BOOL WINAPI CreateProcessAsUserA(_In_opt_ HANDLE hToken, _In_opt_ LPCSTR lpApplicationName, _Inout_opt_ LPSTR lpCommandLine,
	                                 _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes, _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes,
	                                 _In_ BOOL bInheritHandles, _In_ DWORD dwCreationFlags, _In_opt_ LPVOID lpEnvironment,
	                                 _In_opt_ LPCSTR lpCurrentDirectory, _In_ LPSTARTUPINFOA lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation)
	{
		const BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
		if (!bOrigSuspended)
		{
			dwCreationFlags |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION backup{};
		if (!lpProcessInformation)
		{
			lpProcessInformation = &backup;
		}

		if (!Trampoline(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
		                dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
		{
			return FALSE;
		}

		if (DWORD dwCodeIntegrityLevel = 0;
			hToken && GetTokenIntegrityLevel(hToken, &dwCodeIntegrityLevel))
		{
			// 目前发现 chrome 的沙箱进程貌似有完整性检查
			if (dwCodeIntegrityLevel == SECURITY_MANDATORY_LOW_RID || dwCodeIntegrityLevel == SECURITY_MANDATORY_UNTRUSTED_RID)
			{
				return TRUE;
			}
		}
		const BOOL bRet = inject_dll_to_process(lpProcessInformation);
		if (bRet)
		{
			if (!bOrigSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
			if (lpProcessInformation == &backup)
			{
				CloseHandle(lpProcessInformation->hProcess);
				CloseHandle(lpProcessInformation->hThread);
			}
		}
		else
		{
			TerminateProcess(lpProcessInformation->hProcess, ~0u);
			CloseHandle(lpProcessInformation->hProcess);
			CloseHandle(lpProcessInformation->hThread);
			if (lpProcessInformation != &backup)
			{
				lpProcessInformation->hProcess = nullptr;
				lpProcessInformation->hThread = nullptr;
				lpProcessInformation->dwProcessId = 0;
				lpProcessInformation->dwThreadId = 0;
			}
		}
		return bRet;
	}

	template <auto Trampoline>
	BOOL WINAPI CreateProcessAsUserW(_In_opt_ HANDLE hToken, _In_opt_ LPCWSTR lpApplicationName, _Inout_opt_ LPWSTR lpCommandLine,
	                                 _In_opt_ LPSECURITY_ATTRIBUTES lpProcessAttributes, _In_opt_ LPSECURITY_ATTRIBUTES lpThreadAttributes, _In_ BOOL bInheritHandles,
	                                 _In_ DWORD dwCreationFlags, _In_opt_ LPVOID lpEnvironment, _In_opt_ LPCWSTR lpCurrentDirectory,
	                                 _In_ LPSTARTUPINFOW lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation)
	{
		const BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
		if (!bOrigSuspended)
		{
			dwCreationFlags |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION backup{};
		if (!lpProcessInformation)
		{
			lpProcessInformation = &backup;
		}

		if (!Trampoline(hToken, lpApplicationName, lpCommandLine, lpProcessAttributes, lpThreadAttributes, bInheritHandles,
		                dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
		{
			return FALSE;
		}

		if (DWORD dwCodeIntegrityLevel = 0;
			hToken && GetTokenIntegrityLevel(hToken, &dwCodeIntegrityLevel))
		{
			// 目前发现 chrome 的沙箱进程貌似有完整性检查
			if (dwCodeIntegrityLevel == SECURITY_MANDATORY_LOW_RID || dwCodeIntegrityLevel == SECURITY_MANDATORY_UNTRUSTED_RID)
			{
				return TRUE;
			}
		}
		const BOOL bRet = inject_dll_to_process(lpProcessInformation);
		if (bRet)
		{
			if (!bOrigSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
			if (lpProcessInformation == &backup)
			{
				CloseHandle(lpProcessInformation->hProcess);
				CloseHandle(lpProcessInformation->hThread);
			}
		}
		else
		{
			TerminateProcess(lpProcessInformation->hProcess, ~0u);
			CloseHandle(lpProcessInformation->hProcess);
			CloseHandle(lpProcessInformation->hThread);
			if (lpProcessInformation != &backup)
			{
				lpProcessInformation->hProcess = nullptr;
				lpProcessInformation->hThread = nullptr;
				lpProcessInformation->dwProcessId = 0;
				lpProcessInformation->dwThreadId = 0;
			}
		}
		return bRet;
	}

	template <auto Trampoline>
	BOOL WINAPI CreateProcessWithLogonW(_In_ LPCWSTR lpUsername, _In_opt_ LPCWSTR lpDomain, _In_ LPCWSTR lpPassword,
	                                    _In_ DWORD dwLogonFlags, _In_opt_ LPCWSTR lpApplicationName, _Inout_opt_ LPWSTR lpCommandLine,
	                                    _In_ DWORD dwCreationFlags, _In_opt_ LPVOID lpEnvironment, _In_opt_ LPCWSTR lpCurrentDirectory,
	                                    _In_ LPSTARTUPINFOW lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation)
	{
		const BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
		if (!bOrigSuspended)
		{
			dwCreationFlags |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION backup{};
		if (!lpProcessInformation)
		{
			lpProcessInformation = &backup;
		}

		if (!Trampoline(lpUsername, lpDomain, lpPassword, dwLogonFlags, lpApplicationName, lpCommandLine, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
		{
			return FALSE;
		}

		if (DWORD dwCodeIntegrityLevel = 0;
			GetProcessIntegrityLevel(lpProcessInformation->hProcess, &dwCodeIntegrityLevel))
		{
			if (dwCodeIntegrityLevel == SECURITY_MANDATORY_LOW_RID || dwCodeIntegrityLevel == SECURITY_MANDATORY_UNTRUSTED_RID)
			{
				return TRUE;
			}
		}
		const BOOL bRet = inject_dll_to_process(lpProcessInformation);
		if (bRet)
		{
			if (!bOrigSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
			if (lpProcessInformation == &backup)
			{
				CloseHandle(lpProcessInformation->hProcess);
				CloseHandle(lpProcessInformation->hThread);
			}
		}
		else
		{
			TerminateProcess(lpProcessInformation->hProcess, ~0u);
			CloseHandle(lpProcessInformation->hProcess);
			CloseHandle(lpProcessInformation->hThread);
			if (lpProcessInformation != &backup)
			{
				lpProcessInformation->hProcess = nullptr;
				lpProcessInformation->hThread = nullptr;
				lpProcessInformation->dwProcessId = 0;
				lpProcessInformation->dwThreadId = 0;
			}
		}
		return bRet;
	}

	template <auto Trampoline>
	BOOL WINAPI CreateProcessWithTokenW(_In_ HANDLE hToken, _In_ DWORD dwLogonFlags,
	                                    _In_opt_ LPCWSTR lpApplicationName, _Inout_opt_ LPWSTR lpCommandLine, _In_ DWORD dwCreationFlags,
	                                    _In_opt_ LPVOID lpEnvironment, _In_opt_ LPCWSTR lpCurrentDirectory,
	                                    _In_ LPSTARTUPINFOW lpStartupInfo, _Out_ LPPROCESS_INFORMATION lpProcessInformation)
	{
		const BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
		if (!bOrigSuspended)
		{
			dwCreationFlags |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION backup{};
		if (!lpProcessInformation)
		{
			lpProcessInformation = &backup;
		}

		if (!Trampoline(hToken, dwLogonFlags, lpApplicationName, lpCommandLine, dwCreationFlags, lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
		{
			return FALSE;
		}

		if (DWORD dwCodeIntegrityLevel = 0;
			hToken && GetTokenIntegrityLevel(hToken, &dwCodeIntegrityLevel))
		{
			if (dwCodeIntegrityLevel == SECURITY_MANDATORY_LOW_RID || dwCodeIntegrityLevel == SECURITY_MANDATORY_UNTRUSTED_RID)
			{
				return TRUE;
			}
		}
		const BOOL bRet = inject_dll_to_process(lpProcessInformation);
		if (bRet)
		{
			if (!bOrigSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
			if (lpProcessInformation == &backup)
			{
				CloseHandle(lpProcessInformation->hProcess);
				CloseHandle(lpProcessInformation->hThread);
			}
		}
		else
		{
			TerminateProcess(lpProcessInformation->hProcess, ~0u);
			CloseHandle(lpProcessInformation->hProcess);
			CloseHandle(lpProcessInformation->hThread);
			if (lpProcessInformation != &backup)
			{
				lpProcessInformation->hProcess = nullptr;
				lpProcessInformation->hThread = nullptr;
				lpProcessInformation->dwProcessId = 0;
				lpProcessInformation->dwThreadId = 0;
			}
		}
		return bRet;
	}

	void hook_advapi32()
	{
		// win7 or earlier
		if (!global::Data::get().sysVersion().isWindows8OrGreater)
		{
			create_hook_by_func_ptr<&::RegLoadAppKeyW>().setHook(&RegLoadAppKeyW);
		}

		create_hook_by_func_ptr<&::CreateProcessAsUserA>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&CreateProcessAsUserA<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::CreateProcessAsUserW>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&CreateProcessAsUserW<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::CreateProcessWithLogonW>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&CreateProcessWithLogonW<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::CreateProcessWithTokenW>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&CreateProcessWithTokenW<trampolineConst.value>};
		});
	}
}
