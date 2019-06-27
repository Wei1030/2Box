#pragma once

class CDbghelpWrapper;

class CFileModule
{
public:
	CFileModule();
	~CFileModule(void);

	BOOL Create(HANDLE hFile);
	void Destroy();

	DWORD GetProcRVA(const char* pszProcName,CDbghelpWrapper* pHelper);
	size_t GetProcVA(const char* pszProcName,CDbghelpWrapper* pHelper);

private:
	HANDLE m_hFileMapping;
	void*  m_pMemory;
};
