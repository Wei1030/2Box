#pragma once
#include "BaseHook.h"
#include <Wbemidl.h>

class COle32Hook : public CBaseHook
{
public:
	COle32Hook(void);
	~COle32Hook(void);

	virtual BOOL Init() override;

private:
	static HRESULT STDAPICALLTYPE CoCreateInstance( 
		REFCLSID rclsid, 
		LPUNKNOWN pUnkOuter, 
		DWORD dwClsContext, 
		REFIID riid, 
		LPVOID FAR* ppv );

	static HRESULT STDAPICALLTYPE CoCreateInstanceEx(
		REFCLSID  Clsid,
		IUnknown * punkOuter, // only relevant locally
		DWORD dwClsCtx,
		COSERVERINFO * pServerInfo,
		DWORD dwCount,
		MULTI_QI * pResults );
};
