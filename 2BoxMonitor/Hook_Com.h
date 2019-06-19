#pragma once
#include <Wbemidl.h>

HRESULT STDAPICALLTYPE Hook_CoCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv );

HRESULT STDAPICALLTYPE Hook_CoCreateInstanceEx(REFCLSID  Clsid,
												 IUnknown * punkOuter, // only relevant locally
												 DWORD dwClsCtx,
												 COSERVERINFO * pServerInfo,
												 DWORD dwCount,
												 MULTI_QI * pResults );