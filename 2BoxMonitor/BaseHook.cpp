#include "StdAfx.h"
#include "BaseHook.h"
#include "InitialData.h"
#include "TrampolineFunc.h"

CBaseHook* CBaseHook::s_pHead = NULL;

BOOL CBaseHook::HookAll()
{
	for (CBaseHook* p = s_pHead; p != NULL; p = p->m_pNext) 
	{
		if (FALSE == p->Init())
		{
			return FALSE;
		}
	}

	return CTrampolineFuncBase::HookAll();
}

CBaseHook::CBaseHook(void)
: m_pNext(s_pHead)
, m_bFileCreateSucc(FALSE)
{
	s_pHead = this;
}

CBaseHook::~CBaseHook(void)
{	
}

BOOL CBaseHook::InitFile(const wchar_t* pszSysFileName,BOOL bDenyAcess /*= TRUE*/)
{
	BOOL bValRet = FALSE;
	HANDLE hFile = INVALID_HANDLE_VALUE;

	do 
	{
		wchar_t szTrueFileName[MAX_PATH+1] = {0};
		swprintf_s(szTrueFileName,MAX_PATH,L"%s\\%s.dll",
			g_pData->GetSysPathW(),
			pszSysFileName);			

		hFile =CreateFileW(szTrueFileName,
			GENERIC_READ,
			FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (INVALID_HANDLE_VALUE == hFile)
		{
			break;
		}		

		m_bFileCreateSucc = m_fm.Create(hFile);
		if (bDenyAcess)
		{
			g_pData->AddFilesToMgr(szTrueFileName);
		}		
		bValRet = TRUE;

	} while (0);

	if (INVALID_HANDLE_VALUE != hFile)
	{
		CloseHandle(hFile);
	}
	
	return bValRet;
}

void CBaseHook::UninitFile()
{
	if (m_bFileCreateSucc)
	{
		m_fm.Destroy();
	}	
}

