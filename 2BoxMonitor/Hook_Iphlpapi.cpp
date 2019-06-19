#include "stdafx.h"
#include "Hook_Iphlpapi.h"
#include "TrampolineFunc.h"
#include "InitialData.h"

#pragma comment (lib, "Iphlpapi.lib")

CTrampolineFunc<ULONG(WINAPI*)(PIP_ADAPTER_INFO,PULONG)>
TrueGetAdaptersInfo(&GetAdaptersInfo,&Hook_GetAdaptersInfo);

ULONG WINAPI Hook_GetAdaptersInfo( PIP_ADAPTER_INFO AdapterInfo,PULONG SizePointer )
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

CTrampolineFunc<ULONG(WINAPI*)(ULONG,ULONG,PVOID,PIP_ADAPTER_ADDRESSES,PULONG)>
TrueGetAdaptersAddresses(&GetAdaptersAddresses,&Hook_GetAdaptersAddresses);

ULONG WINAPI Hook_GetAdaptersAddresses( ULONG Family, 
									   ULONG Flags, 
									   PVOID Reserved, 
									   PIP_ADAPTER_ADDRESSES AdapterAddresses, 
									   PULONG SizePointer )
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

// ULONG
// WINAPI Hook_GetTcpTable( OUT PMIB_TCPTABLE TcpTable, IN OUT PULONG SizePointer, IN BOOL Order )
// {
// 	MessageBox(NULL,L"Hook_GetTcpTable",L"",MB_OK);
// 	if (TcpTable)
// 	{
// 		TcpTable->dwNumEntries = 0;
// 	}
// 	return NO_ERROR;
// }
// 
// ULONG
// WINAPI Hook_GetTcpTable2( OUT PMIB_TCPTABLE2 TcpTable, IN OUT PULONG SizePointer, IN BOOL Order )
// {
// 	MessageBox(NULL,L"Hook_GetTcpTable2",L"",MB_OK);
// 	if (TcpTable)
// 	{
// 		TcpTable->dwNumEntries = 0;
// 	}
// 	return NO_ERROR;
// }
// 
// DWORD
// WINAPI Hook_GetExtendedTcpTable( OUT PVOID pTcpTable, IN OUT PDWORD pdwSize, IN BOOL bOrder, 
// 								IN ULONG ulAf, IN TCP_TABLE_CLASS TableClass, IN ULONG Reserved )
// {
// 	MessageBox(NULL,L"Hook_GetExtendedTcpTable",L"",MB_OK);
// 	PMIB_TCPTABLE TcpTable = (PMIB_TCPTABLE)pTcpTable;
// 	if (TcpTable)
// 	{
// 		TcpTable->dwNumEntries = 0;
// 	}
// 	return NO_ERROR;
// }
