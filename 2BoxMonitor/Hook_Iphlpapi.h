#pragma once
#include <WinSock2.h>
#include <Iphlpapi.h>

ULONG WINAPI Hook_GetAdaptersInfo(PIP_ADAPTER_INFO AdapterInfo,PULONG SizePointer);

ULONG WINAPI Hook_GetAdaptersAddresses(
	ULONG Family,
	ULONG Flags,
	PVOID Reserved,
	PIP_ADAPTER_ADDRESSES AdapterAddresses, 
	PULONG SizePointer);


// ULONG
// WINAPI
// Hook_GetTcpTable(
// 			OUT PMIB_TCPTABLE TcpTable,
// 			IN OUT PULONG SizePointer,
// 			IN BOOL Order
// 			);
// 
// ULONG
// WINAPI
// Hook_GetTcpTable2(
// 			 OUT PMIB_TCPTABLE2 TcpTable,
// 			 IN OUT PULONG SizePointer,
// 			 IN BOOL Order
// 			 );
// 
// DWORD
// WINAPI
// Hook_GetExtendedTcpTable(
// 					OUT    PVOID           pTcpTable,
// 					IN OUT PDWORD          pdwSize,
// 					IN     BOOL            bOrder,
// 					IN     ULONG           ulAf,
// 					IN     TCP_TABLE_CLASS TableClass,
// 					IN     ULONG           Reserved
// 					);
