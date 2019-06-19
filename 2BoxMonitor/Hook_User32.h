#pragma once

LRESULT
WINAPI
Hook_DispatchMessageA(
				 __in CONST MSG *lpMsg);

LRESULT
WINAPI
Hook_DispatchMessageW(
				 __in CONST MSG *lpMsg);

HWND
WINAPI
Hook_CreateWindowExA(
				__in DWORD dwExStyle,
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
				__in_opt LPVOID lpParam);

HWND
WINAPI
Hook_CreateWindowExW(
				__in DWORD dwExStyle,
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
				__in_opt LPVOID lpParam);

HWND
WINAPI
Hook_FindWindowA(
			__in_opt LPCSTR lpClassName,
			__in_opt LPCSTR lpWindowName);

HWND
WINAPI
Hook_FindWindowW(
			__in_opt LPCWSTR lpClassName,
			__in_opt LPCWSTR lpWindowName);

HWND
WINAPI
Hook_FindWindowExA(
			  __in_opt HWND hWndParent,
			  __in_opt HWND hWndChildAfter,
			  __in_opt LPCSTR lpszClass,
			  __in_opt LPCSTR lpszWindow);

HWND
WINAPI
Hook_FindWindowExW(
			  __in_opt HWND hWndParent,
			  __in_opt HWND hWndChildAfter,
			  __in_opt LPCWSTR lpszClass,
			  __in_opt LPCWSTR lpszWindow);

BOOL
WINAPI
Hook_EnumWindows(
			__in WNDENUMPROC lpEnumFunc,
			__in LPARAM lParam);

BOOL
WINAPI
Hook_EnumChildWindows(
				 __in_opt HWND hWndParent,
				 __in WNDENUMPROC lpEnumFunc,
				 __in LPARAM lParam);

HWND
WINAPI
Hook_GetWindow(
		  __in HWND hWnd,
		  __in UINT uCmd);

HWND
WINAPI
Hook_GetTopWindow(
			 __in_opt HWND hWnd);