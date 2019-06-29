#include "StdAfx.h"
#include "Netapi32Hook.h"
#include "InitialData.h"
#include "TrampolineFunc.h"

#pragma comment (lib, "Netapi32.lib")

TRAMPOLINE(UCHAR(APIENTRY*)(PNCB),Netbios);

CNetapi32Hook::CNetapi32Hook(void)
{
}

CNetapi32Hook::~CNetapi32Hook(void)
{
}

BOOL CNetapi32Hook::Init()
{
// 	HMODULE hMod = GetModuleHandleW(L"netapi32.dll");
// 	if (NULL == hMod)
// 	{
// 		return TRUE;
// 	}

	CBaseHook::InitFile(L"netapi32");		

	HOOK(CNetapi32Hook,Netbios);

	CBaseHook::UninitFile();
	return TRUE;
}

UCHAR APIENTRY CNetapi32Hook::Netbios(PNCB pcnb)
{
	UCHAR ret = TrueNetbios.Call()(pcnb);

	if (pcnb->ncb_command == NCBASTAT 
		&& pcnb->ncb_buffer
		&& pcnb->ncb_length >= sizeof(ADAPTER_STATUS))
	{
		UCHAR fix = (UCHAR)g_pData->GetllData();
		ADAPTER_STATUS* pAdapter = (ADAPTER_STATUS*)pcnb->ncb_buffer;
		pAdapter->adapter_address[0]+= fix; 
		pAdapter->adapter_address[1]+= fix;
		pAdapter->adapter_address[2]+= fix;
		pAdapter->adapter_address[3]+= fix;
		pAdapter->adapter_address[4]+= fix;
		pAdapter->adapter_address[5]+= fix;
//		MessageBox(NULL,L"Netbios",L"",MB_OK);
	}
	return ret;
}
