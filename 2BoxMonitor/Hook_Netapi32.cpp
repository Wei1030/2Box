#include "stdafx.h"
#include "Hook_Netapi32.h"
#include "InitialData.h"
#include "TrampolineFunc.h"

#pragma comment (lib, "Netapi32.lib")

CTrampolineFunc<UCHAR(APIENTRY*)(PNCB)>
TrueNetbios(&Netbios,Hook_Netbios);

UCHAR APIENTRY Hook_Netbios( PNCB pcnb )
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
