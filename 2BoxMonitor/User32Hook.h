#pragma once
#include "BaseHook.h"

class CUser32Hook : public CBaseHook
{
public:
	CUser32Hook(void);
	~CUser32Hook(void);

	virtual BOOL Init() override;

private:
	static LRESULT
		WINAPI
		DispatchMessageA(
		__in CONST MSG *lpMsg);

	static LRESULT
		WINAPI
		DispatchMessageW(
		__in CONST MSG *lpMsg);

	static HWND
		WINAPI
		CreateWindowExA(
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

	static HWND
		WINAPI
		CreateWindowExW(
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

	static HWND
		WINAPI
		FindWindowA(
		__in_opt LPCSTR lpClassName,
		__in_opt LPCSTR lpWindowName);

	static HWND
		WINAPI
		FindWindowW(
		__in_opt LPCWSTR lpClassName,
		__in_opt LPCWSTR lpWindowName);

	static HWND
		WINAPI
		FindWindowExA(
		__in_opt HWND hWndParent,
		__in_opt HWND hWndChildAfter,
		__in_opt LPCSTR lpszClass,
		__in_opt LPCSTR lpszWindow);

	static HWND
		WINAPI
		FindWindowExW(
		__in_opt HWND hWndParent,
		__in_opt HWND hWndChildAfter,
		__in_opt LPCWSTR lpszClass,
		__in_opt LPCWSTR lpszWindow);

	static BOOL
		WINAPI
		EnumWindows(
		__in WNDENUMPROC lpEnumFunc,
		__in LPARAM lParam);

	static BOOL
		WINAPI
		EnumChildWindows(
		__in_opt HWND hWndParent,
		__in WNDENUMPROC lpEnumFunc,
		__in LPARAM lParam);

	static HWND
		WINAPI
		GetWindow(
		__in HWND hWnd,
		__in UINT uCmd);

	static HWND
		WINAPI
		GetTopWindow(
		__in_opt HWND hWnd);
};
