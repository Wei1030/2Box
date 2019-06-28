#include "StdAfx.h"
#include "FileModule.h"

CFileModule::CFileModule(void)
: m_hFileMapping(NULL)
, m_pMemory(NULL)
, m_pDosHeader(NULL)
, m_pNTHeader(NULL)
, m_pOpHeaderAddr(NULL)
, m_pExportDir(NULL)
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
			PAGE_READONLY | SEC_IMAGE,
			0,0,NULL);

		if (NULL == m_hFileMapping)
		{
			break;
		}

		m_pMemory = MapViewOfFile(m_hFileMapping,FILE_MAP_READ,0,0,0);
		if (NULL == m_pMemory)
		{
			break;
		}		
		m_pDosHeader = (PIMAGE_DOS_HEADER)m_pMemory;
		//pe头地址;
		m_pNTHeader = (PIMAGE_NT_HEADERS)((size_t)m_pMemory + (m_pDosHeader->e_lfanew));
		//可选头;
		m_pOpHeaderAddr = (void*)((size_t)m_pNTHeader + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader));
		//导出表;
		m_pExportDir = (PIMAGE_EXPORT_DIRECTORY)GetImageDirectoryEntry(IMAGE_DIRECTORY_ENTRY_EXPORT,NULL);
		
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
		UnmapViewOfFile(m_pMemory);
		m_pMemory = NULL;
	}

	if (m_hFileMapping)
	{
		CloseHandle(m_hFileMapping);
		m_hFileMapping = NULL;
	}
}

DWORD CFileModule::GetProcRVA(const char* pszProcName)
{
	if (m_pExportDir == NULL)
	{
		return 0;
	}

	DWORD fnRVA = 0;	

	PDWORD pdwNamesRvas = (PDWORD)
		((PBYTE)m_pMemory + m_pExportDir->AddressOfNames);
		//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfNames,&pSectionHeader);
	
	PWORD pdwNameOrdinals = (PWORD)
		((PBYTE)m_pMemory + m_pExportDir->AddressOfNameOrdinals);
		//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfNameOrdinals,&pSectionHeader);

	PDWORD pdwFunctionAddresses = (PDWORD)
		((PBYTE)m_pMemory + m_pExportDir->AddressOfFunctions);
		//pHelper->ImageRvaToVa(pNTHeader,m_pMemory,pExportDir->AddressOfFunctions,&pSectionHeader);

	std::string strFuncName;
	std::string strToFind = pszProcName;

	// Walk the array of this module's function names 
	for (DWORD n = 0; n < m_pExportDir->NumberOfNames; n++) 
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

void* CFileModule::GetImageDirectoryEntry(__in USHORT DirectoryEntry, __out PULONG pOutSize)
{
	WORD wOpMagic = *(WORD*)m_pOpHeaderAddr; // IMAGE_OPTIONAL_HEADER 的 magic字段;
	DWORD Size = 0;
	DWORD VirtualAddress = 0;

	if (wOpMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER32* pOpHeader = (IMAGE_OPTIONAL_HEADER32*)m_pOpHeaderAddr;
		Size = pOpHeader->DataDirectory[DirectoryEntry].Size;
		VirtualAddress = pOpHeader->DataDirectory[DirectoryEntry].VirtualAddress;
	}
	else if (wOpMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		IMAGE_OPTIONAL_HEADER64* pOpHeader = (IMAGE_OPTIONAL_HEADER64*)m_pOpHeaderAddr;
		Size = pOpHeader->DataDirectory[DirectoryEntry].Size;
		VirtualAddress = pOpHeader->DataDirectory[DirectoryEntry].VirtualAddress;
	}

	if (Size == 0 || VirtualAddress == 0)
	{
		return NULL;
	}

	if (pOutSize)
	{
		*pOutSize = Size;
	}

	return (void*)
		((size_t)m_pMemory + VirtualAddress);
}
