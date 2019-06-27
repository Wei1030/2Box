#include "StdAfx.h"
#include "Shell32Hook.h"
#include "InitialData.h"
#include "TrampolineFunc.h"

#pragma comment (lib, "shell32.lib")

TRAMPOLINE(BOOL(STDAPICALLTYPE*)(LPSHELLEXECUTEINFOA),ShellExecuteExA);
TRAMPOLINE(BOOL(STDAPICALLTYPE*)(LPSHELLEXECUTEINFOW),ShellExecuteExW);

CShell32Hook::CShell32Hook(void)
{
}

CShell32Hook::~CShell32Hook(void)
{
}

BOOL CShell32Hook::Init(CDbghelpWrapper* pHelper)
{
	BOOL bValRet = FALSE;

	do 
	{		
		CBaseHook::InitFakeFile(L"shell32");			

		HMODULE hMod = LoadLibraryW(L"shell32.dll");
		if (NULL == hMod)
		{
			break;
		}

		HOOK(CShell32Hook,hMod,ShellExecuteExA,pHelper);
		HOOK(CShell32Hook,hMod,ShellExecuteExW,pHelper);

		bValRet = TRUE;

	} while (0);

	return bValRet;
}

BOOL STDAPICALLTYPE CShell32Hook::ShellExecuteExA(__inout LPSHELLEXECUTEINFOA lpExecInfo)
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

	if (FALSE == ::CreateProcessA(lpExecInfo->lpFile,pszCmd,
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

BOOL STDAPICALLTYPE CShell32Hook::ShellExecuteExW(__inout LPSHELLEXECUTEINFOW lpExecInfo)
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
