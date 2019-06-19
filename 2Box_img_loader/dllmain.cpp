// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "stdafx.h"
#include "resource.h"
#include <process.h>
#include <stdio.h>
#include "MemDll.h"
#include "detours.h"

void loadCheckLibFromResource(HMODULE hLibrary)
{
	LPVOID sRawDll; //资源文件在内存中的地址;
	HRSRC hRes; 
	HGLOBAL hResourceLoaded; 

	if (NULL == hLibrary)
	{
		return;
	}

	// 得到指定的资源文件在内存中的位置 ;
	hRes = FindResource(hLibrary, MAKEINTRESOURCE(IDR_CHECKLIB), RT_RCDATA); 
	if (NULL == hRes)
	{
		return;
	}

	// 将资源文件载入内存 ;
	hResourceLoaded = LoadResource(hLibrary, hRes); 
	if (NULL == hResourceLoaded)
	{
		return;
	}

	// 得到资源文件大小 ;
	DWORD dwSize = SizeofResource(hLibrary, hRes); 

	// 锁定资源以得到它在内存中的地址 ;
	sRawDll = (LPVOID)LockResource(hResourceLoaded); 


	CMemDll memDll;
	//MessageBox(NULL,L"0000",L"0000",MB_OK);
	memDll.LoadLibrary(sRawDll,dwSize);
}

BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
					 )
{
	if (DetourIsHelperProcess()) 
	{
		return TRUE;
	}

	if (DLL_PROCESS_ATTACH == ul_reason_for_call)
	{
		DetourRestoreAfterWith();
		DisableThreadLibraryCalls(hModule);
		loadCheckLibFromResource(hModule);
	}	
	return TRUE;
}

