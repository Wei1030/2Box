#pragma once

class CMemDll
{
public:
	CMemDll(void);
	~CMemDll(void);

	BOOL    LoadLibrary( void *lpFileData , size_t DataLength);
	FARPROC GetProcAddress(LPCSTR lpProcName);
	void	FreeLibrary();

private:
	BOOL CheckData(void *lpFileData, size_t DataLength);
	void*  GetImageBaseAndSize(size_t& imgSize);
	inline size_t  GetAlignedSize(size_t Origin, size_t Alignment){return (Origin + Alignment - 1) / Alignment * Alignment;}
	void CopyDllDatas(void *pDest, void *pSrc);
	void DoRelocation( void *NewBase,size_t nBaseDelta ,DWORD RelocVirtualAddress);
	BOOL FillRavAddress(void *pBase,DWORD dwIatVirtualAddress);

private:
	typedef   BOOL (__stdcall *PFNDllMain)(HINSTANCE, DWORD,  LPVOID );	

private:
	void*					m_pImageBase;
	PIMAGE_DOS_HEADER		m_pDosHeader;
	PIMAGE_NT_HEADERS		m_pNTHeader;
	LPBYTE					m_pOpHeaderAddr;
	WORD					m_wOpMagic;	
	PIMAGE_SECTION_HEADER	m_pSectionHeader;

	PFNDllMain m_pfnDllMain;
};
