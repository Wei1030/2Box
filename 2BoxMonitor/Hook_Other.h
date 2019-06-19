#pragma once
#include <ShellAPI.h>
BOOL
WINAPI
Hook_CreateProcessWithLogonW(
						__in        LPCWSTR lpUsername,
						__in_opt    LPCWSTR lpDomain,
						__in        LPCWSTR lpPassword,
						__in        DWORD dwLogonFlags,
						__in_opt    LPCWSTR lpApplicationName,
						__inout_opt LPWSTR lpCommandLine,
						__in        DWORD dwCreationFlags,
						__in_opt    LPVOID lpEnvironment,
						__in_opt    LPCWSTR lpCurrentDirectory,
						__in        LPSTARTUPINFOW lpStartupInfo,
						__out       LPPROCESS_INFORMATION lpProcessInformation
						);

BOOL
WINAPI
Hook_CreateProcessWithTokenW(
						__in        HANDLE hToken,
						__in        DWORD dwLogonFlags,
						__in_opt    LPCWSTR lpApplicationName,
						__inout_opt LPWSTR lpCommandLine,
						__in        DWORD dwCreationFlags,
						__in_opt    LPVOID lpEnvironment,
						__in_opt    LPCWSTR lpCurrentDirectory,
						__in        LPSTARTUPINFOW lpStartupInfo,
						__out       LPPROCESS_INFORMATION lpProcessInformation
						);

BOOL STDAPICALLTYPE Hook_ShellExecuteExA(__inout LPSHELLEXECUTEINFOA lpExecInfo);
BOOL STDAPICALLTYPE Hook_ShellExecuteExW(__inout LPSHELLEXECUTEINFOW lpExecInfo);