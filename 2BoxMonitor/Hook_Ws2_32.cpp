#include "InitialData.h"
#include "Hook_Ws2_32.h"



#pragma comment (lib, "ws2_32.lib")

INT
WSAAPI Hook_getaddrinfo( __in_opt PCSTR pNodeName, __in_opt PCSTR pServiceName, __in_opt const ADDRINFOA * pHints, __deref_out PADDRINFOA * ppResult )
{
	INT ret = g_pTrueAPIs->Truegetaddrinfo(pNodeName,pServiceName,pHints,ppResult);

	if (pServiceName)
	{
		return ret;
	}

	char sname[256]={0};
	gethostname(sname,256);
	
	if (pNodeName)
	{
		if (0 != strcmp(sname,pNodeName))
		{
			return ret;
		}
	}

	struct addrinfo *res, *cur;
	res = *ppResult;
	struct sockaddr_in *addr;
	UCHAR fix = (UCHAR)g_pData->GetllData();

	for (cur = res; cur != NULL; cur = cur->ai_next) {
		addr = (struct sockaddr_in *)cur->ai_addr;

		if (addr && (*addr).sin_addr.S_un.S_un_b.s_b1 != 0)
		{
			(*addr).sin_addr.S_un.S_un_b.s_b1 = 192;
			(*addr).sin_addr.S_un.S_un_b.s_b2 = 168;
			(*addr).sin_addr.S_un.S_un_b.s_b3 = 0;
			if ((*addr).sin_addr.S_un.S_un_b.s_b4 == fix)
			{
				(*addr).sin_addr.S_un.S_un_b.s_b4 += 1;
			}
			else
			{
				(*addr).sin_addr.S_un.S_un_b.s_b4 = fix;
			}
			fix ++;

// 			wchar_t szip[32] = {0};
// 			swprintf(szip,L"%d.%d.%d.%d",
// 				(*addr).sin_addr.S_un.S_un_b.s_b1,
// 			(*addr).sin_addr.S_un.S_un_b.s_b2,
// 			(*addr).sin_addr.S_un.S_un_b.s_b3,
// 			(*addr).sin_addr.S_un.S_un_b.s_b4);
// 
// 			g_pData->DebugWriteLog(szip,64,L"C:\\Hook_getaddrinfo.txt",0);
		}			
	}

	return ret;
}

struct hostent FAR *
	WSAAPI Hook_gethostbyname( __in const char FAR * name )
{
//	g_pData->DebugWriteLog(L"Hook_gethostbyname",36,L"C:\\Hook_gethostbyname.txt",0);
	return NULL;
}

int
WSAAPI Hook_getsockname( IN SOCKET s, __out_bcount_part(*namelen,*namelen) struct sockaddr FAR * name, __inout int FAR * namelen )
{
	int ret = g_pTrueAPIs->Truegetsockname(s,name,namelen);
	struct sockaddr_in *addr = (struct sockaddr_in *)name;
	
	if (addr && (*addr).sin_addr.S_un.S_un_b.s_b1 != 0)
	{
		UCHAR fix = (UCHAR)g_pData->GetllData();

		(*addr).sin_addr.S_un.S_un_b.s_b1 = 192;
		(*addr).sin_addr.S_un.S_un_b.s_b2 = 168;
		(*addr).sin_addr.S_un.S_un_b.s_b3 = 0;
		if ((*addr).sin_addr.S_un.S_un_b.s_b4 == fix)
		{
			(*addr).sin_addr.S_un.S_un_b.s_b4 += 1;
		}
		else
		{
			(*addr).sin_addr.S_un.S_un_b.s_b4 = fix;
		}
		fix ++;

// 		wchar_t szip[32] = {0};
// 		swprintf(szip,L"%d.%d.%d.%d",
// 			(*addr).sin_addr.S_un.S_un_b.s_b1,
// 			(*addr).sin_addr.S_un.S_un_b.s_b2,
// 			(*addr).sin_addr.S_un.S_un_b.s_b3,
// 			(*addr).sin_addr.S_un.S_un_b.s_b4);
// 
// 		g_pData->DebugWriteLog(szip,64,L"C:\\Hook_getsockname.txt",0);
	}			

	return ret;
}
