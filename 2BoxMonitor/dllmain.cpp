// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "InitialData.h"
#include "HookMgr.h"
#include "DbghelpWrapper.h"
#include "detours.h"

void* g_pClient = NULL;

int SRPC_API Box_Err(void* instance,void* usr)
{
	if (g_pClient == instance)
	{
		TerminateProcess(GetCurrentProcess(),0);
	}
	else
	{
		SRPC_DestroyClient(instance);
	}
	
	return 0;
}

// LONG
// WINAPI
// MyFilter(struct _EXCEPTION_POINTERS * e)
// {
// 	return EXCEPTION_EXECUTE_HANDLER;
// }

int Init()
{
	SRPC_INIT_BEGIN()
	SRPC_INIT_END()
	if (SRPC_INIT_FAIL)
	{		
		TerminateProcess(GetCurrentProcess(),0);
		return -1;
	}

	SRPC_SetConnErrCb(NULL,&Box_Err,NULL);

	g_pClient = SRPC_CreateClient(FD_LOCAL,"2Box_{BE6D2570-E5E5-40f0-9A53-CBDDE9C422BD}",0);
	if (NULL == g_pClient)
	{		
		TerminateProcess(GetCurrentProcess(),0);
		return -1;
	}
	
	return 0;
}

void InitHook()
{
//	SetUnhandledExceptionFilter(MyFilter);	

	if (FALSE == g_pData->Init())
	{
		TerminateProcess(GetCurrentProcess(),0);
		return;
	}

	CHookMgr hookMgr;
	CDbghelpWrapper helper;
	CDbghelpWrapper* pHelper = &helper;

	if (FALSE == helper.Init())
	{
		pHelper = NULL;
	}

	if (FALSE == hookMgr.Init(pHelper))
	{
		TerminateProcess(GetCurrentProcess(),0);
		return;
	}
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	if (DetourIsHelperProcess()) 
	{
		return TRUE;
	}

	if (DLL_PROCESS_ATTACH == ul_reason_for_call)
	{	
		DetourRestoreAfterWith();
		DisableThreadLibraryCalls(hModule);
		Init();
		InitHook();
	}

	return TRUE;
}