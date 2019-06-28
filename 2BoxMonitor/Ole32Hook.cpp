#include "StdAfx.h"
#include "Ole32Hook.h"
#include "TrampolineFunc.h"

#pragma comment (lib, "wbemuuid.lib")

TRAMPOLINE(HRESULT(STDAPICALLTYPE*)(REFCLSID,LPUNKNOWN,DWORD,REFIID,LPVOID FAR*),CoCreateInstance);
TRAMPOLINE(HRESULT(STDAPICALLTYPE*)(REFCLSID,IUnknown*,DWORD,COSERVERINFO*,DWORD,MULTI_QI*),CoCreateInstanceEx);

COle32Hook::COle32Hook(void)
{
}

COle32Hook::~COle32Hook(void)
{
}

BOOL COle32Hook::Init()
{
	CBaseHook::InitFile(L"ole32");	

	BOOL bValRet = FALSE;

	do 
	{
		HMODULE hMod = LoadLibraryW(L"ole32.dll");
		if (NULL == hMod)
		{
			break;
		}

		HOOK(COle32Hook,hMod,CoCreateInstance);
		HOOK(COle32Hook,hMod,CoCreateInstanceEx);

		bValRet = TRUE;

	} while (0);

	CBaseHook::UninitFile();
	return bValRet;
}

HRESULT STDAPICALLTYPE COle32Hook::CoCreateInstance(REFCLSID rclsid, 
													LPUNKNOWN pUnkOuter, 
													DWORD dwClsContext, 
													REFIID riid, 
													LPVOID FAR* ppv)
{
	if (memcmp(&rclsid,&CLSID_WbemLocator,sizeof(CLSID)) == 0)
	{
		//MessageBox(NULL,L"CoCreateInstance",L"",MB_OK);
		return -2;
	}
	return TrueCoCreateInstance.Call()(rclsid,pUnkOuter,dwClsContext,riid,ppv);
}

HRESULT STDAPICALLTYPE COle32Hook::CoCreateInstanceEx(REFCLSID Clsid, 
													  IUnknown * punkOuter, /* only relevant locally */ 
													  DWORD dwClsCtx, 
													  COSERVERINFO * pServerInfo, 
													  DWORD dwCount, 
													  MULTI_QI * pResults)
{
	if (memcmp(&Clsid,&CLSID_WbemLocator,sizeof(CLSID)) == 0)
	{
		//MessageBox(NULL,L"CoCreateInstanceEx",L"",MB_OK);
		return -2;
	}
	return TrueCoCreateInstanceEx.Call()(Clsid,punkOuter,dwClsCtx,pServerInfo,dwCount,pResults);
}
