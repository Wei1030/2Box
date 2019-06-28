#pragma once
#include "BaseHook.h"

#include <WinIoCtl.h>

class CKernel32Hook : public CBaseHook
{
public:
	CKernel32Hook(void);
	~CKernel32Hook(void);

	virtual BOOL Init() override;

private:
	static BOOL WINAPI WaitNamedPipeA(__in LPCSTR lpNamedPipeName,__in DWORD nTimeOut);

	static BOOL WINAPI WaitNamedPipeW(__in LPCWSTR lpNamedPipeName,	__in DWORD nTimeOut);

	static BOOL	WINAPI CreateProcessA(
		__in_opt    LPCSTR lpApplicationName,
		__inout_opt LPSTR lpCommandLine,
		__in_opt    LPSECURITY_ATTRIBUTES lpProcessAttributes,
		__in_opt    LPSECURITY_ATTRIBUTES lpThreadAttributes,
		__in        BOOL bInheritHandles,
		__in        DWORD dwCreationFlags,
		__in_opt    LPVOID lpEnvironment,
		__in_opt    LPCSTR lpCurrentDirectory,
		__in        LPSTARTUPINFOA lpStartupInfo,
		__out       LPPROCESS_INFORMATION lpProcessInformation
		);

	static BOOL WINAPI CreateProcessW(
		__in_opt    LPCWSTR lpApplicationName,
		__inout_opt LPWSTR lpCommandLine,
		__in_opt    LPSECURITY_ATTRIBUTES lpProcessAttributes,
		__in_opt    LPSECURITY_ATTRIBUTES lpThreadAttributes,
		__in        BOOL bInheritHandles,
		__in        DWORD dwCreationFlags,
		__in_opt    LPVOID lpEnvironment,
		__in_opt    LPCWSTR lpCurrentDirectory,
		__in        LPSTARTUPINFOW lpStartupInfo,
		__out       LPPROCESS_INFORMATION lpProcessInformation
		);

	static BOOL WINAPI CreateProcessAsUserA (
		__in_opt    HANDLE hToken,
		__in_opt    LPCSTR lpApplicationName,
		__inout_opt LPSTR lpCommandLine,
		__in_opt    LPSECURITY_ATTRIBUTES lpProcessAttributes,
		__in_opt    LPSECURITY_ATTRIBUTES lpThreadAttributes,
		__in        BOOL bInheritHandles,
		__in        DWORD dwCreationFlags,
		__in_opt    LPVOID lpEnvironment,
		__in_opt    LPCSTR lpCurrentDirectory,
		__in        LPSTARTUPINFOA lpStartupInfo,
		__out       LPPROCESS_INFORMATION lpProcessInformation
		);

	static BOOL	WINAPI CreateProcessAsUserW (
		__in_opt    HANDLE hToken,
		__in_opt    LPCWSTR lpApplicationName,
		__inout_opt LPWSTR lpCommandLine,
		__in_opt    LPSECURITY_ATTRIBUTES lpProcessAttributes,
		__in_opt    LPSECURITY_ATTRIBUTES lpThreadAttributes,
		__in        BOOL bInheritHandles,
		__in        DWORD dwCreationFlags,
		__in_opt    LPVOID lpEnvironment,
		__in_opt    LPCWSTR lpCurrentDirectory,
		__in        LPSTARTUPINFOW lpStartupInfo,
		__out       LPPROCESS_INFORMATION lpProcessInformation
		);

	static UINT	WINAPI WinExec(__in LPCSTR lpCmdLine,__in UINT uCmdShow);

	static BOOL WINAPI AssignProcessToJobObject(HANDLE hJob, HANDLE hProcess);

	static BOOL	WINAPI DeviceIoControl(
		__in        HANDLE hDevice,
		__in        DWORD dwIoControlCode,
		__in_bcount_opt(nInBufferSize) LPVOID lpInBuffer,
		__in        DWORD nInBufferSize,
		__out_bcount_part_opt(nOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer,
		__in        DWORD nOutBufferSize,
		__out_opt   LPDWORD lpBytesReturned,
		__inout_opt LPOVERLAPPED lpOverlapped
		);
};
