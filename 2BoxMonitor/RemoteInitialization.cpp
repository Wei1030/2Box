#include "stdafx.h"
#include "RemoteInitialization.h"

extern void* g_pClient;
extern void* g_helperClient;
extern BOOL	 g_bIsOnX64;

int SRPC_API bResultCallBack(unsigned int error,const void* data,unsigned int size,void* usr)
{
	if (error || NULL == data || size != sizeof(BOOL) || NULL == usr)
	{
		return -1;
	}

	memcpy(usr,data,size);
	return 0;
}

BOOL Is64Process( HANDLE hProcess )
{
	if (FALSE == g_bIsOnX64)
	{
		return FALSE;
	}

	BOOL bOnWow64 = FALSE;
	if (FALSE == IsWow64Process(hProcess,&bOnWow64))
	{
		return FALSE;
	}
	return !bOnWow64;
}

BOOL InitAllDllsByCreateRemoteThread(DWORD dwPid)
{
	BOOL bOk = FALSE; 
	HANDLE hProcess = NULL;
	HANDLE hThread = NULL;

	//init all RemoteDll by CreateRemoteThread
	do 
	{
		hProcess = OpenProcess(
			PROCESS_QUERY_INFORMATION |   // Required by Alpha
			PROCESS_CREATE_THREAD,        // For CreateRemoteThread			
			FALSE, dwPid);
		if (hProcess == NULL)
		{
			break;
		}

		//随便建立一个远程线程，目的只是为了让挂起创建的远程进程进行所有dll的初始化;
		PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
			GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "GetProcessId");

		if (pfnThreadRtn == NULL)
		{
			break;
		}

		// Create a remote thread
		hThread = CreateRemoteThread(hProcess, NULL, 0, 
			pfnThreadRtn, (void*)-1, 0, NULL);
		if (hThread == NULL)
		{
			break;
		}

		// Wait for the remote thread to terminate
		WaitForSingleObject(hThread, INFINITE);

		bOk = TRUE;

	} while (0);	

	if (hThread) 
	{
		CloseHandle(hThread);
	}		

	if (hProcess) 
	{
		CloseHandle(hProcess);
	}
	return bOk;
}

namespace RemoteInitialization
{

#ifdef _WIN64

	BOOL InitAllRemoteDlls(HANDLE hProcess)
	{
		BOOL bRet = FALSE;
		DWORD dwPid = GetProcessId(hProcess);

		if (Is64Process(hProcess))
		{
			bRet = InitAllDllsByCreateRemoteThread(dwPid);
		}
		else
		{
			if (NULL == g_pClient)
			{
				return FALSE;
			}

			if (0 != SRPC_Call(g_pClient,"2Box.InitAllRemoteDlls",
				&dwPid,sizeof(DWORD),
				&bResultCallBack,&bRet,
				10000))
			{
				return FALSE;
			}
		}

		return bRet;
	}	

#else

	BOOL InitAllRemoteDlls(HANDLE hProcess)
	{
		BOOL bRet = FALSE;
		DWORD dwPid = GetProcessId(hProcess);

		if (Is64Process(hProcess))
		{
			if (NULL == g_helperClient)
			{
				return FALSE;
			}

			if (0 != SRPC_Call(g_helperClient,"2Box_x64_helper.InitAllRemoteDlls",
				&dwPid,sizeof(DWORD),
				&bResultCallBack,&bRet,
				10000))
			{
				return FALSE;
			}
		}
		else
		{
			bRet = InitAllDllsByCreateRemoteThread(dwPid);
		}

		return bRet;
	}	

#endif

}