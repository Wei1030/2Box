#include "StdAfx.h"
#include "FileModule.h"
#include "DbghelpWrapper.h"

CFileModule::CFileModule(void)
: m_hFileMapping(NULL)
, m_pMemory(NULL)
{
}

CFileModule::~CFileModule(void)
{
	Destroy();
}

BOOL CFileModule::Create(HANDLE hFile)
{
	BOOL bValRet = FALSE;

	do 
	{
		m_hFileMapping = CreateFileMappingW(hFile,
			NULL,
			PAGE_EXECUTE_READWRITE | SEC_IMAGE,
			0,0,NULL);

		if (NULL == m_hFileMapping)
		{
			break;
		}

		m_pMemory = MapViewOfFile(m_hFileMapping,FILE_MAP_EXECUTE | FILE_MAP_READ | FILE_MAP_WRITE,0,0,0);
		if (NULL == m_pMemory)
		{
			break;
		}

		bValRet = TRUE;
	} while (0);

	if (FALSE == bValRet)
	{
		Destroy();
	}

	return bValRet;
}

void CFileModule::Destroy()
{
	if (m_pMemory)
	{
		FlushViewOfFile(m_pMemory,0);
		UnmapViewOfFile(m_pMemory);
		m_pMemory = NULL;
	}

	if (m_hFileMapping)
	{
		CloseHandle(m_hFileMapping);
		m_hFileMapping = NULL;
	}
}

DWORD CFileModule::GetProcRVA(const char* pszProcName,CDbghelpWrapper* pHelper)
{
	DWORD fnRVA = 0;
	
// 	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)m_pMemory;
// 	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((size_t)m_pMemory + (pDosHeader->e_lfanew));
 	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	ULONG ulSize;
	PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)pHelper->ImageDirectoryEntryToDataEx(
		m_pMemory, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ulSize,&pSectionHeader);	
	if (pExportDir == NULL)
	{
		return 0;
	}

	PDWORD pdwNamesRvas = (PDWORD)
		((PBYTE)m_pMemory + pExportDir->AddressOfNames);
		//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfNames,&pSectionHeader);
	
	PWORD pdwNameOrdinals = (PWORD)
		((PBYTE)m_pMemory + pExportDir->AddressOfNameOrdinals);
		//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfNameOrdinals,&pSectionHeader);

	PDWORD pdwFunctionAddresses = (PDWORD)
		((PBYTE)m_pMemory + pExportDir->AddressOfFunctions);
		//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfFunctions,&pSectionHeader);

	std::string strFuncName;
	std::string strToFind = pszProcName;

	// Walk the array of this module's function names 
	for (DWORD n = 0; n < pExportDir->NumberOfNames; n++) 
	{
		// Get the function name
		strFuncName = (PSTR)
			((PBYTE)m_pMemory + pdwNamesRvas[n]);
			//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pdwNamesRvas[n],&pSectionHeader);			

		if (strFuncName != strToFind)
		{
			continue;
		}

		// We found the specified function
		// --> Get this function's ordinal value
		WORD ordinal = pdwNameOrdinals[n];

		// Get the address of this function's address
		fnRVA = (DWORD)pdwFunctionAddresses[ordinal];
		//没有判断ppfnRVA 是否是 Forwarder RVA; fix later;
		break;  // We did it, get out
	}

	return fnRVA;
}

size_t CFileModule::GetProcVA(const char* pszProcName,CDbghelpWrapper* pHelper)
{
	size_t fnVA = 0;

// 	PIMAGE_DOS_HEADER pDosHeader = (PIMAGE_DOS_HEADER)m_pMemory;
// 	PIMAGE_NT_HEADERS pNTHeader = (PIMAGE_NT_HEADERS)((size_t)m_pMemory + (pDosHeader->e_lfanew));
	PIMAGE_SECTION_HEADER pSectionHeader = NULL;
	ULONG ulSize;
	PIMAGE_EXPORT_DIRECTORY pExportDir = (PIMAGE_EXPORT_DIRECTORY)pHelper->ImageDirectoryEntryToDataEx(
		m_pMemory, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &ulSize,&pSectionHeader);	
	if (pExportDir == NULL)
	{
		return 0;
	}

	PDWORD pdwNamesRvas = (PDWORD)
		((PBYTE)m_pMemory + pExportDir->AddressOfNames);
	//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfNames,&pSectionHeader);

	PWORD pdwNameOrdinals = (PWORD)
		((PBYTE)m_pMemory + pExportDir->AddressOfNameOrdinals);
	//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfNameOrdinals,&pSectionHeader);

	PDWORD pdwFunctionAddresses = (PDWORD)
		((PBYTE)m_pMemory + pExportDir->AddressOfFunctions);
	//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfFunctions,&pSectionHeader);

	std::string strFuncName;
	std::string strToFind = pszProcName;

	// Walk the array of this module's function names 
	for (DWORD n = 0; n < pExportDir->NumberOfNames; n++) 
	{
		// Get the function name
		strFuncName = (PSTR)
			((PBYTE)m_pMemory + pdwNamesRvas[n]);
		//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pdwNamesRvas[n],&pSectionHeader);	

		if (strFuncName != strToFind)
		{
			continue;
		}

		// We found the specified function
		// --> Get this function's ordinal value
		WORD ordinal = pdwNameOrdinals[n];

		// Get the address of this function's address
		//没有判断fnRVA 是否是 Forwarder RVA; fix later;
		DWORD fnRVA = (DWORD)pdwFunctionAddresses[ordinal];
		if (fnRVA)
		{
			fnVA = (size_t)
				((PBYTE)m_pMemory + fnRVA);
				//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,fnRVA,&pSectionHeader);
		}		
		break;  // We did it, get out
	}

	return fnVA;
}
