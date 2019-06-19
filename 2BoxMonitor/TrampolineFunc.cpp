#include "StdAfx.h"
#include "TrampolineFunc.h"
#include "detours.h"

CTrampolineFuncBase* CTrampolineFuncBase::s_pHead = NULL;

HMODULE CTrampolineFuncBase::s_hNtdll = NULL;

CTrampolineFuncBase::CTrampolineFuncBase( PROC sourceFunc,PROC detourFunc )
: m_sourceFunc(sourceFunc)
, m_detourFunc(detourFunc)
, m_pNext(s_pHead)
{
	if (NULL == s_pHead)
	{
		s_hNtdll = GetModuleHandleW(L"ntdll.dll");
	}
	s_pHead = this;
}

CTrampolineFuncBase::CTrampolineFuncBase( LPCWSTR lpLibFileName,LPCSTR lpProcName,PROC detourFunc)
: m_sourceFunc(NULL)
, m_detourFunc(detourFunc)
, m_pNext(s_pHead)
{
	if (NULL == s_pHead)
	{
		s_hNtdll = GetModuleHandleW(L"ntdll.dll");
	}
	s_pHead = this;
	HMODULE hDll = LoadLibraryW(lpLibFileName);
	if (hDll)
	{
		m_sourceFunc = GetProcAddress(hDll,lpProcName);
	}
}

CTrampolineFuncBase::CTrampolineFuncBase(LPCSTR lpNtProcName,PROC detourFunc )
: m_sourceFunc(NULL)
, m_detourFunc(detourFunc)
, m_pNext(s_pHead)
{
	if (NULL == s_pHead)
	{
		s_hNtdll = GetModuleHandleW(L"ntdll.dll");
	}
	s_pHead = this;
	if (s_hNtdll)
	{
		m_sourceFunc = GetProcAddress(s_hNtdll,lpNtProcName);
	}
}

BOOL CTrampolineFuncBase::HookAll()
{
	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	for (CTrampolineFuncBase* p = s_pHead; p != NULL; p = p->m_pNext) 
	{
		if (p->m_sourceFunc && p->m_detourFunc) 
		{
			DetourAttach(&(PVOID&)p->m_sourceFunc, p->m_detourFunc);
		} 
	}

	LONG error = DetourTransactionCommit();
	return (error == NO_ERROR ? TRUE : FALSE);
}

BOOL CTrampolineFuncBase::RestoreThisHook()
{
	if (NULL == m_sourceFunc || NULL == m_detourFunc)
	{
		return FALSE;
	}

	DetourTransactionBegin();
	DetourUpdateThread(GetCurrentThread());

	DetourDetach(&(PVOID&)m_sourceFunc, m_detourFunc);

	LONG error = DetourTransactionCommit();
	return (error == NO_ERROR ? TRUE : FALSE);
}


