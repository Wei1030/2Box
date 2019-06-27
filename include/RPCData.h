#ifndef _RPCDATA_H__
#define _RPCDATA_H__

typedef struct _NEW_PROC_DATA
{
	DWORD		iPid;
	unsigned long long	hNewProcHandleInBox;
	//unsigned long long	hBoxHandleInNewProc;
	//unsigned long long	hHookedEventInNewProc;
	//unsigned long long	llData;
	//char				szLoaderPath[261];
}NEW_PROC_DATA;

typedef struct _EMBED_INFO
{
	DWORD				iPid;
	unsigned long long	hWnd;
	DWORD				dwExStyle;
	DWORD				dwStyle;
	wchar_t				szTitle[260];
}EMBED_INFO;

typedef struct _WND_INFO
{
	DWORD				iPid;
	unsigned long long	hWnd;
}WND_INFO;

#endif // #ifndef _RPCDATA_H__