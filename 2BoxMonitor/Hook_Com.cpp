#include "stdafx.h"
#include "Hook_Com.h"
#include "TrampolineFunc.h"

#pragma comment (lib, "wbemuuid.lib")

CTrampolineFunc<HRESULT(STDAPICALLTYPE*)(REFCLSID,LPUNKNOWN,DWORD,REFIID,LPVOID FAR*)>
TrueCoCreateInstance(&CoCreateInstance,&Hook_CoCreateInstance);

HRESULT STDAPICALLTYPE Hook_CoCreateInstance( REFCLSID rclsid, 
											 LPUNKNOWN pUnkOuter, 
											 DWORD dwClsContext, 
											 REFIID riid, 
											 LPVOID FAR* ppv )
{	
	if (memcmp(&rclsid,&CLSID_WbemLocator,sizeof(CLSID)) == 0)
	{
		//MessageBox(NULL,L"CoCreateInstance",L"",MB_OK);
		return -2;
	}
	return TrueCoCreateInstance.Call()(rclsid,pUnkOuter,dwClsContext,riid,ppv);
}

CTrampolineFunc<HRESULT(STDAPICALLTYPE*)(REFCLSID,IUnknown*,DWORD,COSERVERINFO*,DWORD,MULTI_QI*)>
TrueCoCreateInstanceEx(&CoCreateInstanceEx,&Hook_CoCreateInstanceEx);

HRESULT STDAPICALLTYPE Hook_CoCreateInstanceEx( REFCLSID Clsid, 
											   IUnknown * punkOuter, /* only relevant locally */ 
											   DWORD dwClsCtx,
											   COSERVERINFO * pServerInfo, 
											   DWORD dwCount, 
											   MULTI_QI * pResults )
{
	if (memcmp(&Clsid,&CLSID_WbemLocator,sizeof(CLSID)) == 0)
	{
		//MessageBox(NULL,L"CoCreateInstanceEx",L"",MB_OK);
		return -2;
	}
	return TrueCoCreateInstanceEx.Call()(Clsid,punkOuter,dwClsCtx,pServerInfo,dwCount,pResults);
}
