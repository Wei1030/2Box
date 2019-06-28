#include "StdAfx.h"
#include "User32Hook.h"
#include "TrampolineFunc.h"
#include "RPCData.h"

extern void* g_pClient;

TRAMPOLINE(LRESULT(WINAPI*)(CONST MSG *),DispatchMessageA);
TRAMPOLINE(LRESULT(WINAPI*)(CONST MSG *),DispatchMessageW);

TRAMPOLINE(HWND(WINAPI*)(DWORD,LPCSTR,LPCSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID),CreateWindowExA);
TRAMPOLINE(HWND(WINAPI*)(DWORD,LPCWSTR,LPCWSTR,DWORD,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID),CreateWindowExW);

TRAMPOLINE(HWND(WINAPI*)(LPCSTR,LPCSTR),FindWindowA);
TRAMPOLINE(HWND(WINAPI*)(LPCWSTR,LPCWSTR),FindWindowW);

TRAMPOLINE(HWND(WINAPI*)(HWND,HWND,LPCSTR,LPCSTR),FindWindowExA);
TRAMPOLINE(HWND(WINAPI*)(HWND,HWND,LPCWSTR,LPCWSTR),FindWindowExW);

TRAMPOLINE(BOOL(WINAPI*)(WNDENUMPROC,LPARAM),EnumWindows);

TRAMPOLINE(BOOL(WINAPI*)(HWND,WNDENUMPROC,LPARAM),EnumChildWindows);

TRAMPOLINE(HWND(WINAPI*)(HWND,UINT),GetWindow);

TRAMPOLINE(HWND(WINAPI*)(HWND),GetTopWindow);

int SRPC_API EmbedInfoCallBack(unsigned int error,const void* data,unsigned int size,void* usr)
{
	if (error || NULL == data || size != sizeof(EMBED_INFO)|| NULL == usr)
	{
		return -1;
	}

	memcpy(usr,data,size);
	return 0;
}

CUser32Hook::CUser32Hook(void)
{
}

CUser32Hook::~CUser32Hook(void)
{
}

BOOL CUser32Hook::Init()
{
	CBaseHook::InitFile(L"user32",FALSE);

	BOOL bValRet = FALSE;

	do 
	{
		HMODULE hMod = LoadLibraryW(L"user32.dll");
		if (NULL == hMod)
		{
			break;
		}

		HOOK(CUser32Hook,hMod,DispatchMessageA);
		HOOK(CUser32Hook,hMod,DispatchMessageW);

		HOOK(CUser32Hook,hMod,CreateWindowExA);
		HOOK(CUser32Hook,hMod,CreateWindowExW);

		HOOK(CUser32Hook,hMod,FindWindowA);
		HOOK(CUser32Hook,hMod,FindWindowW);

		HOOK(CUser32Hook,hMod,FindWindowExA);
		HOOK(CUser32Hook,hMod,FindWindowExW);

		HOOK(CUser32Hook,hMod,EnumWindows);
		HOOK(CUser32Hook,hMod,EnumChildWindows);
		HOOK(CUser32Hook,hMod,GetWindow);
		HOOK(CUser32Hook,hMod,GetTopWindow);

		bValRet = TRUE;

	} while (0);

	CBaseHook::UninitFile();
	return bValRet;
}

LRESULT
WINAPI CUser32Hook::DispatchMessageA(__in CONST MSG *lpMsg)
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
	}

	return TrueDispatchMessageA.Call()(lpMsg);
}

LRESULT
WINAPI CUser32Hook::DispatchMessageW(__in CONST MSG *lpMsg)
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
	}

	return TrueDispatchMessageW.Call()(lpMsg);
}

HWND
WINAPI CUser32Hook::CreateWindowExA(__in DWORD dwExStyle, __in_opt LPCSTR lpClassName, __in_opt LPCSTR lpWindowName, 
									__in DWORD dwStyle, __in int X, __in int Y, __in int nWidth, __in int nHeight, 
									__in_opt HWND hWndParent, __in_opt HMENU hMenu, __in_opt HINSTANCE hInstance,
									__in_opt LPVOID lpParam)
{
	HWND hWndRet = TrueCreateWindowExA.Call()(dwExStyle,lpClassName,lpWindowName,dwStyle,
		X,Y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);
	if (NULL == hWndRet)
	{
		return hWndRet;
	}

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

HWND
WINAPI CUser32Hook::CreateWindowExW(__in DWORD dwExStyle, __in_opt LPCWSTR lpClassName, __in_opt LPCWSTR lpWindowName, 
									__in DWORD dwStyle, __in int X, __in int Y, __in int nWidth, __in int nHeight,
									__in_opt HWND hWndParent, __in_opt HMENU hMenu, __in_opt HINSTANCE hInstance,
									__in_opt LPVOID lpParam)
{
	HWND hWndRet = TrueCreateWindowExW.Call()(dwExStyle,lpClassName,lpWindowName,dwStyle,
		X,Y,nWidth,nHeight,hWndParent,hMenu,hInstance,lpParam);
	if (NULL == hWndRet)
	{
		return hWndRet;
	}

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

HWND
WINAPI CUser32Hook::FindWindowA(__in_opt LPCSTR lpClassName, __in_opt LPCSTR lpWindowName)
{
	HWND hRet = TrueFindWindowA.Call()(lpClassName,lpWindowName);
	if (NULL == hRet)
	{
		return hRet;
	}

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

HWND
WINAPI CUser32Hook::FindWindowW(__in_opt LPCWSTR lpClassName, __in_opt LPCWSTR lpWindowName)
{
	HWND hRet = TrueFindWindowW.Call()(lpClassName,lpWindowName);
	if (NULL == hRet)
	{
		return hRet;
	}

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

HWND
WINAPI CUser32Hook::FindWindowExA(__in_opt HWND hWndParent, __in_opt HWND hWndChildAfter, __in_opt LPCSTR lpszClass, 
								  __in_opt LPCSTR lpszWindow)
{
	HWND hRet = TrueFindWindowExA.Call()(hWndParent,hWndChildAfter,lpszClass,lpszWindow);
	if (NULL == hRet)
	{
		return hRet;
	}

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

HWND
WINAPI CUser32Hook::FindWindowExW(__in_opt HWND hWndParent, __in_opt HWND hWndChildAfter, __in_opt LPCWSTR lpszClass, 
								  __in_opt LPCWSTR lpszWindow)
{
	HWND hRet = TrueFindWindowExW.Call()(hWndParent,hWndChildAfter,lpszClass,lpszWindow);
	if (NULL == hRet)
	{
		return hRet;
	}

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

BOOL
WINAPI CUser32Hook::EnumWindows(__in WNDENUMPROC lpEnumFunc, __in LPARAM lParam)
{
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

BOOL
WINAPI CUser32Hook::EnumChildWindows(__in_opt HWND hWndParent, __in WNDENUMPROC lpEnumFunc, __in LPARAM lParam)
{
	if (hWndParent)
	{
		return TrueEnumChildWindows.Call()(hWndParent,lpEnumFunc,lParam);
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

HWND
WINAPI CUser32Hook::GetWindow(__in HWND hWnd, __in UINT uCmd)
{
	HWND hRet = TrueGetWindow.Call()(hWnd,uCmd);
	if (NULL == hRet)
	{
		return hRet;
	}

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

HWND
WINAPI CUser32Hook::GetTopWindow(__in_opt HWND hWnd)
{
	if (hWnd)
	{
		return TrueGetTopWindow.Call()(hWnd);
	}	

	HWND hRet = TrueGetTopWindow.Call()(hWnd);
	if (NULL == hRet)
	{
		return hRet;
	}

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
