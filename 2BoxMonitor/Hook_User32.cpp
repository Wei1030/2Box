#include "stdafx.h"
#include "Hook_User32.h"
#include "TrampolineFunc.h"
#include "RPCData.h"

//#include <strsafe.h>

extern void* g_pClient;

int SRPC_API EmbedInfoCallBack(unsigned int error,const void* data,unsigned int size,void* usr)
{
	if (error || NULL == data || size != sizeof(EMBED_INFO)|| NULL == usr)
	{
		return -1;
	}

	memcpy(usr,data,size);
	return 0;
}

CTrampolineFunc<LRESULT(WINAPI*)(CONST MSG *)> TrueDispatchMessageA(&DispatchMessageA,&Hook_DispatchMessageA);

LRESULT
WINAPI Hook_DispatchMessageA(__in CONST MSG *lpMsg)
{
	if (lpMsg
		&& WM_SYSCOMMAND == lpMsg->message)
	{
		if (0xEFFF == lpMsg->wParam
			&& IsWindowVisible(lpMsg->hwnd))
		{
			EMBED_INFO embed_info = {0};
			embed_info.iPid = GetCurrentProcessId();
			embed_info.hWnd = (unsigned long long)lpMsg->hwnd;

			GetWindowText(lpMsg->hwnd,embed_info.szTitle,260);

			SRPC_AsyncCall(g_pClient,"2Box.EmbedWnd",&embed_info,sizeof(embed_info),NULL,NULL);
		}
// 		else if (0xEFFE == lpMsg->wParam)
// 		{
// 			g_pTrueAPIs->CloseDubiousHandles();
// 		}		
	}

	return TrueDispatchMessageA.Call()(lpMsg);
}

CTrampolineFunc<LRESULT(WINAPI*)(CONST MSG *)> TrueDispatchMessageW(&DispatchMessageW,&Hook_DispatchMessageW);

LRESULT
WINAPI Hook_DispatchMessageW(__in CONST MSG *lpMsg)
{
	if (lpMsg
		&& WM_SYSCOMMAND == lpMsg->message)
	{
		if (0xEFFF == lpMsg->wParam
			&& IsWindowVisible(lpMsg->hwnd))
		{
			EMBED_INFO embed_info = {0};
			embed_info.iPid = GetCurrentProcessId();
			embed_info.hWnd = (unsigned long long)lpMsg->hwnd;

			GetWindowText(lpMsg->hwnd,embed_info.szTitle,260);

			SRPC_AsyncCall(g_pClient,"2Box.EmbedWnd",&embed_info,sizeof(embed_info),NULL,NULL);
		}	
// 		else if (0xEFFE == lpMsg->wParam)
// 		{
// 			g_pTrueAPIs->CloseDubiousHandles();
// 		}
	}

	return TrueDispatchMessageW.Call()(lpMsg);
}

CTrampolineFunc<HWND(WINAPI*)(DWORD,LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID)>
TrueCreateWindowExA(&CreateWindowExA,&Hook_CreateWindowExA);

HWND
WINAPI Hook_CreateWindowExA( __in DWORD dwExStyle, 
							__in_opt LPCSTR lpClassName, 
							__in_opt LPCSTR lpWindowName,
							__in DWORD dwStyle, 
							__in int X, 
							__in int Y, 
							__in int nWidth,
							__in int nHeight,
							__in_opt HWND hWndParent,
							__in_opt HMENU hMenu, 
							__in_opt HINSTANCE hInstance, 
							__in_opt LPVOID lpParam )
{
	
	HWND hWndRet = TrueCreateWindowExA.Call()(dwExStyle,lpClassName,lpWindowName,dwStyle,
			X,Y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);

	if (hWndParent || (dwExStyle & WS_EX_TOOLWINDOW))
	{
		return hWndRet;
	}

	HMENU hSysMenu = GetSystemMenu(hWndRet,FALSE);
	if (NULL == hSysMenu)
	{
		return hWndRet;
	}
	
	if (FALSE == AppendMenu(hSysMenu,MF_SEPARATOR,0,NULL))
	{
		return hWndRet;
	}	

	if (FALSE == AppendMenu(hSysMenu,MF_STRING,0xEFFF,_T("扔进盒子")))
	{
		return hWndRet;
	}

	EMBED_INFO embed_info = {0};
	embed_info.iPid = GetCurrentProcessId();
	embed_info.hWnd = (unsigned long long)hWndRet;
	embed_info.dwExStyle = dwExStyle;
	embed_info.dwStyle = dwStyle;

	SRPC_Call(g_pClient,"2Box.NewWnd",&embed_info,sizeof(embed_info),NULL,NULL);
	return hWndRet;
}

CTrampolineFunc<HWND(WINAPI*)(DWORD,LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID)>
TrueCreateWindowExW(&CreateWindowExW,&Hook_CreateWindowExW);

HWND
WINAPI Hook_CreateWindowExW( __in DWORD dwExStyle, 
							__in_opt LPCWSTR lpClassName, 
							__in_opt LPCWSTR lpWindowName, 
							__in DWORD dwStyle, 
							__in int X, 
							__in int Y, 
							__in int nWidth, 
							__in int nHeight,
							__in_opt HWND hWndParent,
							__in_opt HMENU hMenu, 
							__in_opt HINSTANCE hInstance, 
							__in_opt LPVOID lpParam )
{
	HWND hWndRet = TrueCreateWindowExW.Call()(dwExStyle,lpClassName,lpWindowName,dwStyle,
			X,Y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);

	if (hWndParent || (dwExStyle & WS_EX_TOOLWINDOW))
	{
		return hWndRet;
	}

	HMENU hSysMenu = GetSystemMenu(hWndRet,FALSE);
	if (NULL == hSysMenu)
	{
		return hWndRet;
	}

	if (FALSE == AppendMenu(hSysMenu,MF_SEPARATOR,0,NULL))
	{
		return hWndRet;
	}	

	if (FALSE == AppendMenu(hSysMenu,MF_STRING,0xEFFF,_T("扔进盒子")))
	{
		return hWndRet;
	}

	EMBED_INFO embed_info = {0};
	embed_info.iPid = GetCurrentProcessId();
	embed_info.hWnd = (unsigned long long)hWndRet;
	embed_info.dwExStyle = dwExStyle;
	embed_info.dwStyle = dwStyle;

	SRPC_Call(g_pClient,"2Box.NewWnd",&embed_info,sizeof(embed_info),NULL,NULL);
	return hWndRet;	
}

typedef struct _MyEnumWndParams
{
	WNDENUMPROC lpEnumFunc;
	LPARAM		lParam;
	WND_INFO*	pWndInfo;
	size_t		nCount;
}MyEnumWndParams;

int SRPC_API AllWndCallBack(unsigned int error,const void* data,unsigned int size,void* usr)
{
	if (error || NULL == data  || 0 == size || NULL == usr)
	{
		return -1;
	}

	MyEnumWndParams* p = (MyEnumWndParams*)usr;
	p->nCount = size / sizeof(WND_INFO);
	p->pWndInfo = new WND_INFO[p->nCount];
	memcpy(p->pWndInfo,data,size);
	return 0;
}

CTrampolineFunc<HWND(WINAPI*)(LPCSTR,LPCSTR)> TrueFindWindowA(&FindWindowA,&Hook_FindWindowA);

HWND
WINAPI Hook_FindWindowA( __in_opt LPCSTR lpClassName, __in_opt LPCSTR lpWindowName )
{
	if (NULL == g_pClient)
	{
		return NULL;
	}

	HWND hRet = TrueFindWindowA.Call()(lpClassName,lpWindowName);

	MyEnumWndParams MyParams = {0};

	SRPC_Call(g_pClient,"2Box.GetAllWnd",NULL,0,&AllWndCallBack,&MyParams);

	DWORD dwPid = GetCurrentProcessId();

	if (MyParams.pWndInfo && MyParams.nCount)
	{
		for(size_t i = 0; i< MyParams.nCount;++i)
		{
			if (MyParams.pWndInfo[i].iPid != dwPid && MyParams.pWndInfo[i].hWnd == (unsigned long long)hRet)
			{
				hRet = NULL;
				break;
			}			
		}

		delete[] MyParams.pWndInfo;
	}

	return hRet;
}

CTrampolineFunc<HWND(WINAPI*)(LPCWSTR,LPCWSTR)> TrueFindWindowW(&FindWindowW,&Hook_FindWindowW);

HWND
WINAPI Hook_FindWindowW( __in_opt LPCWSTR lpClassName, __in_opt LPCWSTR lpWindowName )
{
	if (NULL == g_pClient)
	{
		return NULL;
	}

	HWND hRet = TrueFindWindowW.Call()(lpClassName,lpWindowName);

	MyEnumWndParams MyParams = {0};

	SRPC_Call(g_pClient,"2Box.GetAllWnd",NULL,0,&AllWndCallBack,&MyParams);

	DWORD dwPid = GetCurrentProcessId();

	if (MyParams.pWndInfo && MyParams.nCount)
	{
		for(size_t i = 0; i< MyParams.nCount;++i)
		{
			if (MyParams.pWndInfo[i].iPid != dwPid && MyParams.pWndInfo[i].hWnd == (unsigned long long)hRet)
			{
				hRet = NULL;
				break;
			}			
		}

		delete[] MyParams.pWndInfo;
	}

	return hRet;
}

CTrampolineFunc<HWND(WINAPI*)(HWND,HWND,LPCSTR,LPCSTR)> TrueFindWindowExA(&FindWindowExA,&Hook_FindWindowExA);

HWND
WINAPI Hook_FindWindowExA( __in_opt HWND hWndParent, __in_opt HWND hWndChildAfter, __in_opt LPCSTR lpszClass, __in_opt LPCSTR lpszWindow )
{
	if (NULL == g_pClient)
	{
		return NULL;
	}

	HWND hRet = TrueFindWindowExA.Call()(hWndParent,hWndChildAfter,lpszClass,lpszWindow);

	MyEnumWndParams MyParams = {0};

	SRPC_Call(g_pClient,"2Box.GetAllWnd",NULL,0,&AllWndCallBack,&MyParams);

	DWORD dwPid = GetCurrentProcessId();

	if (MyParams.pWndInfo && MyParams.nCount)
	{
		for(size_t i = 0; i< MyParams.nCount;++i)
		{
			if (MyParams.pWndInfo[i].iPid != dwPid && MyParams.pWndInfo[i].hWnd == (unsigned long long)hRet)
			{
				hRet = NULL;
				break;
			}			
		}

		delete[] MyParams.pWndInfo;
	}

	return hRet;
}

CTrampolineFunc<HWND(WINAPI*)(HWND,HWND,LPCWSTR,LPCWSTR)> TrueFindWindowExW(&FindWindowExW,&Hook_FindWindowExW);

HWND
WINAPI Hook_FindWindowExW( __in_opt HWND hWndParent, __in_opt HWND hWndChildAfter, __in_opt LPCWSTR lpszClass, __in_opt LPCWSTR lpszWindow )
{
	if (NULL == g_pClient)
	{
		return NULL;
	}

	HWND hRet = TrueFindWindowExW.Call()(hWndParent,hWndChildAfter,lpszClass,lpszWindow);

	MyEnumWndParams MyParams = {0};

	SRPC_Call(g_pClient,"2Box.GetAllWnd",NULL,0,&AllWndCallBack,&MyParams);

	DWORD dwPid = GetCurrentProcessId();

	if (MyParams.pWndInfo && MyParams.nCount)
	{
		for(size_t i = 0; i< MyParams.nCount;++i)
		{
			if (MyParams.pWndInfo[i].iPid != dwPid && MyParams.pWndInfo[i].hWnd == (unsigned long long)hRet)
			{
				hRet = NULL;
				break;
			}			
		}

		delete[] MyParams.pWndInfo;
	}

	return hRet;
}

BOOL CALLBACK MyEnumWindowsProc( HWND hwnd, LPARAM lParam)
{
	static DWORD dwPid = GetCurrentProcessId();
	MyEnumWndParams* p = (MyEnumWndParams*)lParam;
	if (NULL == p || NULL == p->lpEnumFunc)
	{
		return FALSE;
	}

	if (p->pWndInfo && p->nCount)
	{
		for(size_t i = 0; i< p->nCount;++i)
		{
			if (p->pWndInfo[i].iPid != dwPid && p->pWndInfo[i].hWnd == (unsigned long long)hwnd)
			{
				return TRUE;
			}			
		}
	}

	return p->lpEnumFunc(hwnd,p->lParam);
}

CTrampolineFunc<BOOL(WINAPI*)(WNDENUMPROC,LPARAM)> TrueEnumWindows(&EnumWindows,&Hook_EnumWindows);

BOOL
WINAPI Hook_EnumWindows( __in WNDENUMPROC lpEnumFunc, __in LPARAM lParam )
{
	if (NULL == g_pClient)
	{
		return TRUE;
	}

	MyEnumWndParams MyParams = {0};
	MyParams.lpEnumFunc = lpEnumFunc;
	MyParams.lParam = lParam;

	SRPC_Call(g_pClient,"2Box.GetAllWnd",NULL,0,&AllWndCallBack,&MyParams);

	BOOL bRet = TrueEnumWindows.Call()(&MyEnumWindowsProc,(LPARAM)&MyParams);
	if (MyParams.pWndInfo)
	{
		delete[] MyParams.pWndInfo;
	}

	return bRet;
}

CTrampolineFunc<BOOL(WINAPI*)(HWND,WNDENUMPROC,LPARAM)> TrueEnumChildWindows(&EnumChildWindows,&Hook_EnumChildWindows);

BOOL
WINAPI Hook_EnumChildWindows( __in_opt HWND hWndParent, __in WNDENUMPROC lpEnumFunc, __in LPARAM lParam )
{
	if (hWndParent)
	{
		return TrueEnumChildWindows.Call()(hWndParent,lpEnumFunc,lParam);
	}

	if (NULL == g_pClient)
	{
		return TRUE;
	}

	MyEnumWndParams MyParams = {0};
	MyParams.lpEnumFunc = lpEnumFunc;
	MyParams.lParam = lParam;

	SRPC_Call(g_pClient,"2Box.GetAllWnd",NULL,0,&AllWndCallBack,&MyParams);

	BOOL bRet = TrueEnumChildWindows.Call()(hWndParent,&MyEnumWindowsProc,(LPARAM)&MyParams);
	if (MyParams.pWndInfo)
	{
		delete[] MyParams.pWndInfo;
	}

	return bRet;
}

CTrampolineFunc<HWND(WINAPI*)(HWND,UINT)> TrueGetWindow(&GetWindow,&Hook_GetWindow);

HWND
WINAPI Hook_GetWindow( __in HWND hWnd, __in UINT uCmd )
{
	if (NULL == g_pClient)
	{
		return NULL;
	}

	HWND hRet = TrueGetWindow.Call()(hWnd,uCmd);

	MyEnumWndParams MyParams = {0};

	SRPC_Call(g_pClient,"2Box.GetAllWnd",NULL,0,&AllWndCallBack,&MyParams);

	DWORD dwPid = GetCurrentProcessId();

	if (MyParams.pWndInfo && MyParams.nCount)
	{
		for(size_t i = 0; i< MyParams.nCount;++i)
		{
			if (MyParams.pWndInfo[i].iPid != dwPid && MyParams.pWndInfo[i].hWnd == (unsigned long long)hRet)
			{
				hRet = NULL;
				break;
			}			
		}

		delete[] MyParams.pWndInfo;
	}

	return hRet;
}

CTrampolineFunc<HWND(WINAPI*)(HWND)> TrueGetTopWindow(&GetTopWindow,&Hook_GetTopWindow);

HWND
WINAPI Hook_GetTopWindow( __in_opt HWND hWnd )
{
	if (hWnd)
	{
		return TrueGetTopWindow.Call()(hWnd);
	}

	if (NULL == g_pClient)
	{
		return NULL;
	}

	HWND hRet = TrueGetTopWindow.Call()(hWnd);

	MyEnumWndParams MyParams = {0};

	SRPC_Call(g_pClient,"2Box.GetAllWnd",NULL,0,&AllWndCallBack,&MyParams);

	DWORD dwPid = GetCurrentProcessId();

	if (MyParams.pWndInfo && MyParams.nCount)
	{
		for(size_t i = 0; i< MyParams.nCount;++i)
		{
			if (MyParams.pWndInfo[i].iPid != dwPid && MyParams.pWndInfo[i].hWnd == (unsigned long long)hRet)
			{
				hRet = NULL;
				break;
			}			
		}

		delete[] MyParams.pWndInfo;
	}

	return hRet;
}
