#include "StdAfx.h"
#include "DbghelpWrapper.h"

#include <Dbghelp.h>

CDbghelpWrapper::CDbghelpWrapper(void)
: m_hDbgHelp(NULL)
, m_pImageDirectoryEntryToDataEx(NULL)
, m_pImageRvaToVa(NULL)
{
}

CDbghelpWrapper::~CDbghelpWrapper(void)
{
	Uninit();
}

bool CDbghelpWrapper::Init()
{
	bool valRet = false;

	do 
	{
		m_hDbgHelp = LoadLibraryW(L"Dbghelp.dll");
		if (NULL == m_hDbgHelp)
		{
			break;
		}
 
		m_pImageDirectoryEntryToDataEx = (pfnImageDirectoryEntryToDataEx)
			GetProcAddress(m_hDbgHelp,"ImageDirectoryEntryToDataEx");
		if (NULL == m_pImageDirectoryEntryToDataEx)
		{
			break;
		}

		m_pImageRvaToVa = (pfnImageRvaToVa)GetProcAddress(m_hDbgHelp,"ImageRvaToVa");
		if (NULL == m_pImageRvaToVa)
		{
			break;
		}

		valRet = true;
	} while (0);

	if (false == valRet
		&& m_hDbgHelp)
	{
		FreeLibrary(m_hDbgHelp);
		m_hDbgHelp = NULL;
	}

	return valRet;
}

void CDbghelpWrapper::Uninit()
{
	if (m_hDbgHelp)
	{
		FreeLibrary(m_hDbgHelp);
		m_hDbgHelp = NULL;	
	}

	m_pImageDirectoryEntryToDataEx = NULL;
	m_pImageRvaToVa = NULL;
}