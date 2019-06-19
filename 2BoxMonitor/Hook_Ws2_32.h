#pragma once

INT
WSAAPI
Hook_getaddrinfo(
			__in_opt        PCSTR               pNodeName,
			__in_opt        PCSTR               pServiceName,
			__in_opt        const ADDRINFOA *   pHints,
			__deref_out     PADDRINFOA *        ppResult
			); 

struct hostent FAR *
	WSAAPI
	Hook_gethostbyname(
	__in const char FAR * name
	);

int
WSAAPI
Hook_getsockname(
			IN SOCKET s,
			__out_bcount_part(*namelen,*namelen) struct sockaddr FAR * name,
			__inout int FAR * namelen
			);