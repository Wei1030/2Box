#include "StdAfx.h"
#include "Advapi32Hook.h"
#include "TrampolineFunc.h"

TRAMPOLINE(BOOL(WINAPI*)(__in LPCWSTR , __in_opt LPCWSTR , __in LPCWSTR ,
		   __in DWORD , __in_opt LPCWSTR , 
		   __inout_opt LPWSTR , __in DWORD , 
		   __in_opt LPVOID , __in_opt LPCWSTR , 
		   __in LPSTARTUPINFOW , 
		   __out LPPROCESS_INFORMATION ),CreateProcessWithLogonW);

TRAMPOLINE(BOOL(WINAPI*)(__in HANDLE, __in DWORD,
		   __in_opt LPCWSTR, __inout_opt LPWSTR, 
		   __in DWORD, __in_opt LPVOID, 
		   __in_opt LPCWSTR, __in LPSTARTUPINFOW,
		   __out LPPROCESS_INFORMATION),CreateProcessWithTokenW);

CAdvapi32Hook::CAdvapi32Hook(void)
{
}

CAdvapi32Hook::~CAdvapi32Hook(void)
{
}

BOOL CAdvapi32Hook::Init()
{
	HMODULE hMod = GetModuleHandleW(L"advapi32.dll");

	CBaseHook::InitFile(L"advapi32");

	BOOL bValRet = TRUE;		

	HOOK(CAdvapi32Hook,CreateProcessWithLogonW);
	HOOK_SRC_FROM_MEM(CAdvapi32Hook,hMod,CreateProcessWithTokenW);

	CBaseHook::UninitFile();
	return TRUE;
}

BOOL WINAPI CAdvapi32Hook::CreateProcessWithLogonW(__in LPCWSTR lpUsername, __in_opt LPCWSTR lpDomain, 
												   __in LPCWSTR lpPassword, __in DWORD dwLogonFlags, 
												   __in_opt LPCWSTR lpApplicationName, __inout_opt LPWSTR lpCommandLine, 
												   __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment, 
												   __in_opt LPCWSTR lpCurrentDirectory, __in LPSTARTUPINFOW lpStartupInfo, 
												   __out LPPROCESS_INFORMATION lpProcessInformation)
{
	return ::CreateProcessW(lpApplicationName,lpCommandLine,NULL,NULL,FALSE,
		dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
}

BOOL WINAPI CAdvapi32Hook::CreateProcessWithTokenW(__in HANDLE hToken, __in DWORD dwLogonFlags, 
												   __in_opt LPCWSTR lpApplicationName, __inout_opt LPWSTR lpCommandLine, 
												   __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment,
												   __in_opt LPCWSTR lpCurrentDirectory, __in LPSTARTUPINFOW lpStartupInfo, 
												   __out LPPROCESS_INFORMATION lpProcessInformation)
{
	return ::CreateProcessW(lpApplicationName,lpCommandLine,NULL,NULL,FALSE,
		dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
}


