// dllmain.cpp : ���� DLL Ӧ�ó������ڵ㡣
#include "stdafx.h"
#include "resource.h"
#include <process.h>
#include <stdio.h>
#include "MemDll.h"
#include "detours.h"

void loadCheckLibFromResource(HMODULE hLibrary)
{
	LPVOID sRawDll; //��Դ�ļ����ڴ��еĵ�ַ;
	HRSRC hRes; 
	HGLOBAL hResourceLoaded; 

	if (NULL == hLibrary)
	{
		return;
	}

	// �õ�ָ������Դ�ļ����ڴ��е�λ�� ;
	hRes = FindResource(hLibrary, MAKEINTRESOURCE(IDR_CHECKLIB), RT_RCDATA); 
	if (NULL == hRes)
	{
		return;
	}

	// ����Դ�ļ������ڴ� ;
	hResourceLoaded = LoadResource(hLibrary, hRes); 
	if (NULL == hResourceLoaded)
	{
		return;
	}

	// �õ���Դ�ļ���С ;
	DWORD dwSize = SizeofResource(hLibrary, hRes); 

	// ������Դ�Եõ������ڴ��еĵ�ַ ;
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

