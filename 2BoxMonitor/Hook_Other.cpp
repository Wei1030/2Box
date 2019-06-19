#include "stdafx.h"
#include "Hook_Other.h"
#include "InitialData.h"
#include "TrampolineFunc.h"

#pragma comment (lib, "shell32.lib")

CTrampolineFunc<BOOL(WINAPI*)(__in LPCWSTR , __in_opt LPCWSTR , __in LPCWSTR ,
							  __in DWORD , __in_opt LPCWSTR , 
							  __inout_opt LPWSTR , __in DWORD , 
							  __in_opt LPVOID , __in_opt LPCWSTR , 
							  __in LPSTARTUPINFOW , 
							  __out LPPROCESS_INFORMATION )>
TrueCreateProcessWithLogonW(L"Advapi32.dll","CreateProcessWithLogonW",&Hook_CreateProcessWithLogonW);

BOOL
WINAPI Hook_CreateProcessWithLogonW( __in LPCWSTR lpUsername, __in_opt LPCWSTR lpDomain, __in LPCWSTR lpPassword,
									__in DWORD dwLogonFlags, __in_opt LPCWSTR lpApplicationName, 
									__inout_opt LPWSTR lpCommandLine, __in DWORD dwCreationFlags, 
									__in_opt LPVOID lpEnvironment, __in_opt LPCWSTR lpCurrentDirectory, 
									__in LPSTARTUPINFOW lpStartupInfo, 
									__out LPPROCESS_INFORMATION lpProcessInformation )
{
	return CreateProcessW(lpApplicationName,lpCommandLine,NULL,NULL,FALSE,
		dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
}

CTrampolineFunc<BOOL(WINAPI*)(__in HANDLE, __in DWORD,
							  __in_opt LPCWSTR, __inout_opt LPWSTR, 
							  __in DWORD, __in_opt LPVOID, 
							  __in_opt LPCWSTR, __in LPSTARTUPINFOW,
							  __out LPPROCESS_INFORMATION)>
TrueCreateProcessWithTokenW(L"Advapi32.dll","CreateProcessWithTokenW",&Hook_CreateProcessWithTokenW);

BOOL
WINAPI Hook_CreateProcessWithTokenW( __in HANDLE hToken, __in DWORD dwLogonFlags,
									__in_opt LPCWSTR lpApplicationName, __inout_opt LPWSTR lpCommandLine, 
									__in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment, 
									__in_opt LPCWSTR lpCurrentDirectory, __in LPSTARTUPINFOW lpStartupInfo,
									__out LPPROCESS_INFORMATION lpProcessInformation )
{
	return CreateProcessW(lpApplicationName,lpCommandLine,NULL,NULL,FALSE,
		dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
}

CTrampolineFunc<BOOL(STDAPICALLTYPE*)(LPSHELLEXECUTEINFOA)>
TrueShellExecuteExA(&ShellExecuteExA,&Hook_ShellExecuteExA);

BOOL STDAPICALLTYPE Hook_ShellExecuteExA( __inout LPSHELLEXECUTEINFOA lpExecInfo )
{
	PROCESS_INFORMATION piProcInfo;  
	STARTUPINFOA siStartupInfo;  
	ZeroMemory( &siStartupInfo, sizeof(siStartupInfo) );  
	siStartupInfo.cb = sizeof(siStartupInfo);
	siStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	siStartupInfo.wShowWindow = lpExecInfo->nShow;

	char szCmd[MAX_PATH] = {0};
	LPSTR pszCmd = NULL;
	if (lpExecInfo->lpParameters)
	{
		strcpy_s(szCmd,lpExecInfo->lpParameters);
		pszCmd = szCmd;
	}	

	if (FALSE == CreateProcessA(lpExecInfo->lpFile,pszCmd,
		NULL,NULL,FALSE,CREATE_DEFAULT_ERROR_MODE,
		NULL,lpExecInfo->lpDirectory,&siStartupInfo,&piProcInfo))
	{
		return FALSE;
	}

	if (piProcInfo.hThread)
	{
		CloseHandle(piProcInfo.hThread);
	}

	if (piProcInfo.hProcess)
	{
		CloseHandle(piProcInfo.hProcess);
	}

	return TRUE;
}

CTrampolineFunc<BOOL(STDAPICALLTYPE*)(LPSHELLEXECUTEINFOW)>
TrueShellExecuteExW(&ShellExecuteExW,&Hook_ShellExecuteExW);

BOOL STDAPICALLTYPE Hook_ShellExecuteExW( __inout LPSHELLEXECUTEINFOW lpExecInfo )
{
	PROCESS_INFORMATION piProcInfo;  
	STARTUPINFOW siStartupInfo;  
	ZeroMemory( &siStartupInfo, sizeof(siStartupInfo) );  
	siStartupInfo.cb = sizeof(siStartupInfo);
	siStartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	siStartupInfo.wShowWindow = lpExecInfo->nShow;

	 wchar_t szCmd[MAX_PATH] = {0};
	 LPWSTR pszCmd = NULL;
	 if (lpExecInfo->lpParameters)
	 {
		 wcscpy_s(szCmd,lpExecInfo->lpParameters);
		 pszCmd = szCmd;
	 }	 

	if (FALSE == CreateProcessW(lpExecInfo->lpFile,pszCmd,
		NULL,NULL,FALSE,CREATE_DEFAULT_ERROR_MODE,
		NULL,lpExecInfo->lpDirectory,&siStartupInfo,&piProcInfo))
	{
		return FALSE;
	}
	
	if (piProcInfo.hThread)
	{
		CloseHandle(piProcInfo.hThread);
	}

	if (piProcInfo.hProcess)
	{
		CloseHandle(piProcInfo.hProcess);
	}

	return TRUE;
}
