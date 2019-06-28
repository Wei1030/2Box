#pragma once

class CFileModule
{
public:
	CFileModule();
	~CFileModule(void);

	BOOL Create(HANDLE hFile);
	void Destroy();

	DWORD GetProcRVA(const char* pszProcName);	

private:
	void* GetImageDirectoryEntry(__in USHORT DirectoryEntry, __out PULONG pOutSize);

private:
	HANDLE m_hFileMapping;
	void*  m_pMemory;

	PIMAGE_DOS_HEADER m_pDosHeader;
	PIMAGE_NT_HEADERS m_pNTHeader;
	void*	m_pOpHeaderAddr;

	PIMAGE_EXPORT_DIRECTORY m_pExportDir;
};
