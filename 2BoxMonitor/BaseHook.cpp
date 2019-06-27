#include "StdAfx.h"
#include "BaseHook.h"
#include "InitialData.h"
#include "TrampolineFunc.h"

CBaseHook* CBaseHook::s_pHead = NULL;

BOOL CBaseHook::HookAll(CDbghelpWrapper* pHelper)
{
	for (CBaseHook* p = s_pHead; p != NULL; p = p->m_pNext) 
	{
		if (FALSE == p->Init(pHelper))
		{
			return FALSE;
		}
	}

	BOOL bValRet = CTrampolineFuncBase::HookAll();

	for (CBaseHook* p = s_pHead; p != NULL; p = p->m_pNext) 
	{
		p->SavedAndUnmapFakeFile();
	}
	return bValRet;
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

BOOL CBaseHook::InitFakeFile(const wchar_t* pszSysFileName)
{
	BOOL bValRet = FALSE;
	HANDLE hFakeFile = INVALID_HANDLE_VALUE;

	do 
	{		
		wchar_t szFakeFileName[MAX_PATH+1] = {0};
		swprintf_s(szFakeFileName,MAX_PATH,L"%s2BoxFileSystem\\%s_%d.dll",
			g_pData->GetSelfPathW(),
			pszSysFileName,
			GetCurrentProcessId());

		wchar_t szTrueFileName[MAX_PATH+1] = {0};
		swprintf_s(szTrueFileName,MAX_PATH,L"%s\\%s.dll",
			g_pData->GetSysPathW(),
			pszSysFileName);

		if (FALSE == CopyFileW(szTrueFileName,szFakeFileName,TRUE))
		{
			break;
		}		

		hFakeFile =CreateFileW(szFakeFileName,
			GENERIC_EXECUTE | GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);

		if (INVALID_HANDLE_VALUE == hFakeFile)
		{
			break;
		}		

		m_bFileCreateSucc = m_fm.Create(hFakeFile);
		g_pData->AddFilesToMgr(szTrueFileName,szFakeFileName);
		bValRet = TRUE;

	} while (0);

	if (INVALID_HANDLE_VALUE != hFakeFile)
	{
		CloseHandle(hFakeFile);
	}
	
	return bValRet;
}

void CBaseHook::SavedAndUnmapFakeFile()
{
	if (m_bFileCreateSucc)
	{
		m_fm.Destroy();
	}	
}

