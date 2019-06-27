#pragma once

class CDbghelpWrapper
{
public:
	CDbghelpWrapper(void);
	~CDbghelpWrapper(void);

	bool Init();
	void Uninit();

	typedef PVOID (WINAPI* pfnImageDirectoryEntryToDataEx)(
		PVOID Base,
		BOOLEAN MappedAsImage,
		USHORT DirectoryEntry,
		PULONG Size,
		PIMAGE_SECTION_HEADER* FoundHeader);

	inline PVOID ImageDirectoryEntryToDataEx(
		PVOID Base,
		BOOLEAN MappedAsImage,
		USHORT DirectoryEntry,
		PULONG Size,
		PIMAGE_SECTION_HEADER* FoundHeader)
	{
		return m_pImageDirectoryEntryToDataEx(Base,MappedAsImage,DirectoryEntry,Size,FoundHeader);
	}

	typedef PVOID (WINAPI* pfnImageRvaToVa)(
		PIMAGE_NT_HEADERS NtHeaders,
		PVOID Base,
		ULONG Rva,
		PIMAGE_SECTION_HEADER* LastRvaSection);

	inline PVOID ImageRvaToVa(
		PIMAGE_NT_HEADERS NtHeaders,
		PVOID Base,
		ULONG Rva,
		PIMAGE_SECTION_HEADER *LastRvaSection)
	{
		return m_pImageRvaToVa(NtHeaders,Base,Rva,LastRvaSection);
	}
private:
	HMODULE m_hDbgHelp;

	pfnImageDirectoryEntryToDataEx m_pImageDirectoryEntryToDataEx;
	pfnImageRvaToVa m_pImageRvaToVa;
};
