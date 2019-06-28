#pragma once
#include "BaseHook.h"
#include <WinSock2.h>
#include <Iphlpapi.h>

class CIphlpapiHook : public CBaseHook
{
public:
	CIphlpapiHook(void);
	~CIphlpapiHook(void);

	virtual BOOL Init() override;

private:
	static ULONG WINAPI GetAdaptersInfo(PIP_ADAPTER_INFO AdapterInfo,PULONG SizePointer);

	static ULONG WINAPI GetAdaptersAddresses(
		ULONG Family,
		ULONG Flags,
		PVOID Reserved,
		PIP_ADAPTER_ADDRESSES AdapterAddresses, 
		PULONG SizePointer);
};
