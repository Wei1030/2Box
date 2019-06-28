#pragma once
#include "BaseHook.h"

class CAdvapi32Hook :public CBaseHook
{
public:
	CAdvapi32Hook(void);
	~CAdvapi32Hook(void);

	virtual BOOL Init() override;

private:
	static BOOL WINAPI CreateProcessWithLogonW(
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

	static BOOL WINAPI CreateProcessWithTokenW(
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
};
