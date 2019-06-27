#include "StdAfx.h"
#include "TrampolineFunc.h"
#include "detours.h"

CTrampolineFuncBase* CTrampolineFuncBase::s_pHead = NULL;

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

CTrampolineFuncBase::CTrampolineFuncBase( PROC sourceFunc,PROC detourFunc )
: m_sourceFunc(sourceFunc)
, m_detourFunc(detourFunc)
, m_pNext(s_pHead)
{	
	s_pHead = this;
}

CTrampolineFuncBase::CTrampolineFuncBase()
: m_sourceFunc(NULL)
, m_detourFunc(NULL)
, m_pNext(s_pHead)
{
	s_pHead = this;
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


