#include "StdAfx.h"
#include "IphlpapiHook.h"
#include "TrampolineFunc.h"
#include "InitialData.h"

#pragma comment (lib, "Iphlpapi.lib")

TRAMPOLINE(ULONG(WINAPI*)(PIP_ADAPTER_INFO,PULONG),GetAdaptersInfo);
TRAMPOLINE(ULONG(WINAPI*)(ULONG,ULONG,PVOID,PIP_ADAPTER_ADDRESSES,PULONG),GetAdaptersAddresses);

CIphlpapiHook::CIphlpapiHook(void)
{
}

CIphlpapiHook::~CIphlpapiHook(void)
{
}

BOOL CIphlpapiHook::Init()
{
	CBaseHook::InitFile(L"iphlpapi");

	BOOL bValRet = FALSE;

	do 
	{
		HMODULE hMod = LoadLibraryW(L"iphlpapi.dll");
		if (NULL == hMod)
		{
			break;
		}

		HOOK(CIphlpapiHook,hMod,GetAdaptersInfo);
		HOOK(CIphlpapiHook,hMod,GetAdaptersAddresses);

		bValRet = TRUE;

	} while (0);

	CBaseHook::UninitFile();
	return bValRet;
}

ULONG WINAPI CIphlpapiHook::GetAdaptersInfo(PIP_ADAPTER_INFO AdapterInfo,PULONG SizePointer)
{
	ULONG l = TrueGetAdaptersInfo.Call()(AdapterInfo,SizePointer);

	if (l == NO_ERROR)
	{
		BYTE fix = (BYTE)g_pData->GetllData();
		PIP_ADAPTER_INFO pCurrAddresses = AdapterInfo;
		while (pCurrAddresses)
		{
			if (pCurrAddresses->AddressLength != 0)
			{
				pCurrAddresses->Address[0] += fix;
				pCurrAddresses->Address[1] += fix;
				pCurrAddresses->Address[2] += fix;
				pCurrAddresses->Address[3] += fix;
				pCurrAddresses->Address[4] += fix;
				pCurrAddresses->Address[5] += fix;
			}

			pCurrAddresses = pCurrAddresses->Next;
		}
//		MessageBox(NULL,L"GetAdaptersInfo",L"",MB_OK);
	}
	return l;
}

ULONG WINAPI CIphlpapiHook::GetAdaptersAddresses(ULONG Family, ULONG Flags, PVOID Reserved, 
												 PIP_ADAPTER_ADDRESSES AdapterAddresses, PULONG SizePointer)
{
	ULONG l = TrueGetAdaptersAddresses.Call()(
		Family,Flags,Reserved,AdapterAddresses,SizePointer);

	if (l == NO_ERROR)
	{
		BYTE fix = (BYTE)g_pData->GetllData();
		PIP_ADAPTER_ADDRESSES pCurrAddresses = AdapterAddresses;
		while (pCurrAddresses)
		{
			if (pCurrAddresses->PhysicalAddressLength != 0)
			{
				pCurrAddresses->PhysicalAddress[0] += fix;
				pCurrAddresses->PhysicalAddress[1] += fix;
				pCurrAddresses->PhysicalAddress[2] += fix;
				pCurrAddresses->PhysicalAddress[3] += fix;
				pCurrAddresses->PhysicalAddress[4] += fix;
				pCurrAddresses->PhysicalAddress[5] += fix;
			}

			pCurrAddresses = pCurrAddresses->Next;
		}
//		MessageBox(NULL,L"GetAdaptersAddresses",L"",MB_OK);
	}
	return l;
}
