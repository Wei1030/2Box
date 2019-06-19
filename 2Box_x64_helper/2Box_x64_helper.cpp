// 2Box_x64_helper.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "sRPC/sRPC.h"
//#include "Toolhelp.h"

#pragma comment(lib,"sRPC.lib")

LONG g_lClientCount = 0;

BOOL CheckInstance()
{
	if (FALSE == WaitNamedPipe(_T("\\\\.\\pipe\\2Box_x64{BE6D2570-E5E5-40f0-9A53-CBDDE9C422BD}"),1))
	{
		return FALSE;
	}
	return TRUE;
}

int SRPC_API OnAcptEvent(void* instance,int iType,void* usr)
{
	if (0 == iType)
	{
		InterlockedIncrement(&g_lClientCount);
	}
	else
	{
		InterlockedDecrement(&g_lClientCount);
	}

	return 0;
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

// BOOL UnmapRemoteModule(DWORD dwPid,PCTSTR pszModuleName)
// {
// 	BOOL bOk = FALSE; 
// 	HANDLE hProcess = NULL;
// 	HANDLE hThread = NULL;
// 
// 	do 
// 	{
// 		CToolhelp thModules(TH32CS_SNAPMODULE|TH32CS_SNAPMODULE32, dwPid);
// 		MODULEENTRY32 me = { sizeof(me) };
// 		BOOL mFound = FALSE;
// 		for (BOOL fOk = thModules.ModuleFirst(&me); 
// 			fOk; 
// 			fOk = thModules.ModuleNext(&me)) 
// 		{
// 			if (_tcscmp(me.szModule, pszModuleName) == 0) 
// 			{
// 				mFound = TRUE;
// 				break;
// 			}
// 		}
// 
// 		if (FALSE == mFound)
// 		{
// 			break;
// 		}
// 
// 		hProcess = OpenProcess(
// 			PROCESS_QUERY_INFORMATION |   // Required by Alpha
// 			PROCESS_CREATE_THREAD,        // For CreateRemoteThread			
// 			FALSE, dwPid);
// 		if (hProcess == NULL)
// 		{
// 			break;
// 		}
// 
// 		PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)
// 			GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "UnmapViewOfFile");
// 
// 		if (pfnThreadRtn == NULL)
// 		{
// 			break;
// 		}
// 
// 		// Create a remote thread
// 		hThread = CreateRemoteThread(hProcess, NULL, 0, 
// 			pfnThreadRtn, me.hModule, 0, NULL);
// 		if (hThread == NULL)
// 		{
// 			break;
// 		}
// 
// 		// Wait for the remote thread to terminate
// 		WaitForSingleObject(hThread, INFINITE);
// 
// 		bOk = TRUE;
// 	} while (0);
// 
// 	if (hThread) 
// 	{
// 		CloseHandle(hThread);
// 	}		
// 
// 	if (hProcess) 
// 	{
// 		CloseHandle(hProcess);
// 
// 	}
// 	return bOk;
// }

int MODULE_API InitAllRemoteDlls( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
{
	int iPid = 0;
	if (data_in && size_in == sizeof(int))
	{
		memcpy(&iPid,data_in,size_in);
	}
	else
	{
		return -1;
	}

	if (FALSE == InitAllDllsByCreateRemoteThread(iPid))
	{
		return -1;
	}

	if (proc)
	{
		BOOL bRet = TRUE;
		proc(0,&bRet,sizeof(BOOL),usr);
	}
	return 0;
}

// int MODULE_API UnmapRemoteModuleAPI( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
// {
// 	int iPid = 0;
// 	if (data_in && size_in == sizeof(int))
// 	{
// 		memcpy(&iPid,data_in,size_in);
// 	}
// 	else
// 	{
// 		return -1;
// 	}
// 
// 	if (FALSE == UnmapRemoteModule(iPid,_T("2Box_img_loader64.dll")))
// 	{
// 		return -1;
// 	}
// 
// 	if (proc)
// 	{
// 		BOOL bRet = TRUE;
// 		proc(0,&bRet,sizeof(BOOL),usr);
// 	}
// 	return 0;
// }

int APIENTRY _tWinMain(HINSTANCE hInstance,
					   HINSTANCE hPrevInstance,
					   LPTSTR    lpCmdLine,
					   int       nCmdShow)
{
	if (CheckInstance())
	{
		return -1;
	}
	
	SRPC_INIT_BEGIN()
		EXPORT_PROC("2Box_x64_helper.InitAllRemoteDlls",&InitAllRemoteDlls)
		//EXPORT_PROC("2Box_x64_helper.UnmapRemoteModule",&UnmapRemoteModuleAPI)
	SRPC_INIT_END()

	if (SRPC_INIT_FAIL)
	{
		return -1;
	}

	SRPC_SetAcptEventCb(0,&OnAcptEvent,NULL);
	
	void* rpcServer = SRPC_CreateServer(FD_LOCAL,"2Box_x64{BE6D2570-E5E5-40f0-9A53-CBDDE9C422BD}",0);
	if (NULL == rpcServer)
	{
		SRPC_Uninit();
		return -1;
	}

	do 
	{
		Sleep(10000);
	} while (g_lClientCount);

	SRPC_DestroyServer(rpcServer);
	SRPC_Uninit();
	return 0;
}

