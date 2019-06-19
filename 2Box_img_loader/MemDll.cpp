#include "StdAfx.h"
#include "MemDll.h"
#include <stdio.h>

CMemDll::CMemDll(void)
: m_pImageBase(0)
, m_pDosHeader(NULL)
, m_pNTHeader(NULL)
, m_pOpHeaderAddr(NULL)
, m_wOpMagic(0)
, m_pSectionHeader(NULL)
, m_pfnDllMain(NULL)
{
}

CMemDll::~CMemDll(void)
{
}

BOOL CMemDll::LoadLibrary(void *lpFileData , size_t DataLength)
{
	if (m_pImageBase)
	{
		return FALSE;  //已经加载一个dll，还没有释放，不能加载新的dll;
	}

	//检查数据有效性，并初始化;
	if (!CheckData(lpFileData, DataLength))
	{
		return FALSE;
	}

	//计算所需的加载空间;
	size_t ImageSize = 0;
	void* pNeed = GetImageBaseAndSize(ImageSize);
	if (NULL == pNeed || ImageSize == 0)
	{
		return FALSE;
	}

	// 分配虚拟内存;
	void *pMemoryAddress = VirtualAlloc((LPVOID)pNeed, ImageSize,
		MEM_COMMIT | MEM_RESERVE , PAGE_EXECUTE_READWRITE);
	if (pMemoryAddress == NULL)
	{
		pMemoryAddress = VirtualAlloc(NULL, ImageSize,
			MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN, PAGE_EXECUTE_READWRITE);
		if (NULL == pMemoryAddress)
		{
			return FALSE;
		}	
	}

	CopyDllDatas(pMemoryAddress, lpFileData); //复制dll数据，并对齐每个段;

	size_t	nBaseDelta = 0;
	DWORD	dwRelocVirtualAddress = 0;
	DWORD	dwRelocSize = 0;
	DWORD	dwIatVirtualAddress = 0;

	if (IMAGE_NT_OPTIONAL_HDR32_MAGIC == m_wOpMagic)
	{
		IMAGE_OPTIONAL_HEADER32* pOpHeader = (IMAGE_OPTIONAL_HEADER32*)m_pOpHeaderAddr;
		nBaseDelta = (size_t)pMemoryAddress - pOpHeader->ImageBase;
		dwRelocVirtualAddress = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
		dwRelocSize = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size ;
		dwIatVirtualAddress = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress ;

		//修正基地址;
		pOpHeader->ImageBase = (size_t)pMemoryAddress;
		m_pfnDllMain = (PFNDllMain)(pOpHeader->AddressOfEntryPoint + (size_t) pMemoryAddress);
	}
	else
	{
		IMAGE_OPTIONAL_HEADER64* pOpHeader = (IMAGE_OPTIONAL_HEADER64*)m_pOpHeaderAddr;
		nBaseDelta = (size_t)pMemoryAddress - pOpHeader->ImageBase;
		dwRelocVirtualAddress = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress;
		dwRelocSize = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size ;
		dwIatVirtualAddress = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress ;

		//修正基地址;
		pOpHeader->ImageBase = (size_t)pMemoryAddress;
		m_pfnDllMain = (PFNDllMain)(pOpHeader->AddressOfEntryPoint + (size_t) pMemoryAddress);
	}

	//重定位信息;
	if (nBaseDelta && dwRelocVirtualAddress > 0 && dwRelocSize > 0)
	{
		DoRelocation(pMemoryAddress,nBaseDelta,dwRelocVirtualAddress);
	}

	//填充引入地址表;
	if (!FillRavAddress(pMemoryAddress,dwIatVirtualAddress))  //修正引入地址表失败;
	{
		VirtualFree(pMemoryAddress, 0, MEM_RELEASE);
		return FALSE;
	}

	//修改页属性。应该根据每个页的属性单独设置其对应内存页的属性; 
	//fix me later
// 	unsigned long old;
// 	VirtualProtect(pMemoryAddress, ImageSize, PAGE_EXECUTE_READWRITE, &old);
	
	m_pImageBase = pMemoryAddress;

	BOOL InitResult = m_pfnDllMain((HINSTANCE)pMemoryAddress, DLL_PROCESS_ATTACH, 0);
	if (!InitResult)  //初始化失败;
	{
		FreeLibrary();
		return FALSE;
	}

	return TRUE;
}

BOOL CMemDll::CheckData(void *lpFileData, size_t DataLength)
{
	//检查长度;
	if (DataLength < sizeof(IMAGE_DOS_HEADER))
	{
		return FALSE;
	}
	m_pDosHeader = (PIMAGE_DOS_HEADER)lpFileData;  // DOS头
	//检查dos头的标记;
	if (m_pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
	{
		return FALSE;    //0x5A4D : MZ
	}

	//取得pe头;
	m_pNTHeader = (PIMAGE_NT_HEADERS)( (size_t)lpFileData + m_pDosHeader->e_lfanew); // PE头

	size_t dwFileHeaderEndOffset = m_pDosHeader->e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader);
	//检查长度;
	if (DataLength < dwFileHeaderEndOffset ) 
	{
		return FALSE;
	}

	//检查pe头的合法性;
	if (m_pNTHeader->Signature != IMAGE_NT_SIGNATURE)
	{
		return FALSE;    //0x00004550 : PE00
	}

	WORD Characteristics = m_pNTHeader->FileHeader.Characteristics;
	if ((Characteristics & IMAGE_FILE_DLL) == 0) //0x2000  : File is a DLL
	{
		return FALSE;
	}
	if ((Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE) == 0) //0x0002 : 指出文件可以运行;
	{
		return FALSE;
	}
#ifdef _WIN64
	if ( (Characteristics & IMAGE_FILE_32BIT_MACHINE) //设置了适应32位系统，说明一定是32位文件;
		|| ((Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) == 0))//或者不能处理大于2GB地址空间;
	{
		return FALSE;
	}
#else
	if ( ((Characteristics & IMAGE_FILE_32BIT_MACHINE) == 0 )//没有设置适应32位系统;
		&& (Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE))//并且可以处理大于2GB地址空间;
	{
		return FALSE;
	}
#endif

	//检查长度;
	if (DataLength < dwFileHeaderEndOffset + m_pNTHeader->FileHeader.SizeOfOptionalHeader)
	{
		return FALSE;
	}

	m_pOpHeaderAddr = (LPBYTE)((size_t)lpFileData + dwFileHeaderEndOffset);
	m_wOpMagic = *(WORD*)m_pOpHeaderAddr; // IMAGE_OPTIONAL_HEADER 的 magic字段;

	if (m_wOpMagic != IMAGE_NT_OPTIONAL_HDR32_MAGIC
		&& m_wOpMagic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
	{
		return FALSE;
	}
	
	//取得节表（段表）;
	m_pSectionHeader =  IMAGE_FIRST_SECTION( m_pNTHeader );
	//验证每个节表的空间;
	for (WORD i = 0; i < m_pNTHeader->FileHeader.NumberOfSections; i++)
	{
		if ((size_t)(m_pSectionHeader[i].PointerToRawData + m_pSectionHeader[i].SizeOfRawData) > DataLength)
		{
			return FALSE;
		}
	}
	return TRUE;
}

void* CMemDll::GetImageBaseAndSize(size_t& imgSize)
{
	size_t nAlign	= 0;
	void*  pBase	= NULL;
	imgSize			= 0;

	if (IMAGE_NT_OPTIONAL_HDR32_MAGIC == m_wOpMagic)
	{
		nAlign	= ((IMAGE_OPTIONAL_HEADER32*)m_pOpHeaderAddr)->SectionAlignment; //段对齐字节数;
		pBase	= (void*)((IMAGE_OPTIONAL_HEADER32*)m_pOpHeaderAddr)->ImageBase;
	}
	else
	{
		nAlign	= ((IMAGE_OPTIONAL_HEADER64*)m_pOpHeaderAddr)->SectionAlignment; //段对齐字节数;
		pBase	= (void*)((IMAGE_OPTIONAL_HEADER64*)m_pOpHeaderAddr)->ImageBase;
	}

	imgSize = (size_t)m_pSectionHeader - (size_t)m_pDosHeader + 
		sizeof(IMAGE_SECTION_HEADER) * m_pNTHeader->FileHeader.NumberOfSections;

	DWORD CodeSize	= 0;
	DWORD LoadSize	= 0;
	DWORD MaxSize	= 0;
	size_t SectionSize = 0;

	// 计算所有头的尺寸。包括dos, coff, pe头 和 段表的大小;
	imgSize = GetAlignedSize(imgSize, nAlign);
	// 计算所有节的大小;
	for (WORD i = 0; i < m_pNTHeader->FileHeader.NumberOfSections; ++i)
	{
		//得到该节的大小;
		CodeSize = m_pSectionHeader[i].Misc.VirtualSize ;
		LoadSize = m_pSectionHeader[i].SizeOfRawData;
		MaxSize = (LoadSize > CodeSize) ? (LoadSize) : (CodeSize);

		SectionSize = GetAlignedSize(m_pSectionHeader[i].VirtualAddress + MaxSize, nAlign);
		if (imgSize < SectionSize)
		{
			imgSize = SectionSize;    //Use the Max;
		}
	}
	return pBase;
}

void CMemDll::CopyDllDatas(void *pDest, void *pSrc)
{
	// 计算需要复制的PE头+段表字节数;
	size_t  MoveSize = (size_t)m_pSectionHeader - (size_t)m_pDosHeader + 
		sizeof(IMAGE_SECTION_HEADER) * m_pNTHeader->FileHeader.NumberOfSections;

	//复制头和段信息;
	memmove(pDest, pSrc, MoveSize);

	void *pSectionAddress = NULL;

	//复制每个节;
	for (WORD i = 0; i < m_pNTHeader->FileHeader.NumberOfSections; ++i)
	{
		if (m_pSectionHeader[i].VirtualAddress == 0 || m_pSectionHeader[i].SizeOfRawData == 0)
		{
			continue;
		}
		// 定位该节在内存中的位置;
		pSectionAddress = (void *)((size_t)pDest + m_pSectionHeader[i].VirtualAddress);
		// 复制段数据到虚拟内存;
		memmove((void *)pSectionAddress,
			(void *)((size_t)pSrc + m_pSectionHeader[i].PointerToRawData),
			m_pSectionHeader[i].SizeOfRawData);
	}

	//修正指针，指向新分配的内存;
	//新的dos头;
	m_pDosHeader = (PIMAGE_DOS_HEADER)pDest;
	//新的pe头地址;
	m_pNTHeader = (PIMAGE_NT_HEADERS)((size_t)pDest + (m_pDosHeader->e_lfanew));
	//新的可选头;
	m_pOpHeaderAddr = (LPBYTE)((size_t)m_pNTHeader + FIELD_OFFSET(IMAGE_NT_HEADERS, OptionalHeader));
	//新的节表地址;
	m_pSectionHeader = IMAGE_FIRST_SECTION( m_pNTHeader );
	return ;
}

void CMemDll::DoRelocation(void *NewBase,size_t nBaseDelta ,DWORD RelocVirtualAddress)
{
	if ( 0 == nBaseDelta)
	{
		return;
	}

    PIMAGE_BASE_RELOCATION pLoc = (PIMAGE_BASE_RELOCATION)((size_t)NewBase + RelocVirtualAddress);
	WORD *pLocData = NULL;
	int NumberOfReloc = 0;
	DWORD offset = 0;
	WORD type = 0;
	size_t *pAddress = NULL;

	while ((pLoc->VirtualAddress + pLoc->SizeOfBlock) != 0)  //开始扫描重定位表;
    {
        pLocData = (WORD *)((size_t)pLoc + sizeof(IMAGE_BASE_RELOCATION));
        //计算本节需要修正的重定位项（地址）的数目;
        NumberOfReloc = (pLoc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);

        for ( int i = 0 ; i < NumberOfReloc; i++)
        {
			type	= pLocData[i] & 0xF000;	//高四位是有效判断位;
			offset	= pLocData[i] & 0x0FFF;	//低四位是偏移地址;
			
            if ( type)  //这是一个需要修正的地址;
            {
                pAddress = (size_t *)((size_t)NewBase + pLoc->VirtualAddress + offset);
                *pAddress += nBaseDelta;
            }
        }
        //转移到下一个节进行处理;
        pLoc = (PIMAGE_BASE_RELOCATION)((size_t)pLoc + pLoc->SizeOfBlock);
    }
}

BOOL CMemDll::FillRavAddress(void *pBase,DWORD Offset)
{
	if (Offset == 0)
	{
		return TRUE;    //No Import Table
	}

	int i;
	char*	dllName;
	wchar_t buf[MAX_PATH+1]; //dll name;
	PIMAGE_THUNK_DATA pRealIAT;
	PIMAGE_THUNK_DATA pOriginalIAT;
	HMODULE hDll;
	FARPROC lpFunction;
	PIMAGE_IMPORT_DESCRIPTOR pID = (PIMAGE_IMPORT_DESCRIPTOR)((size_t) pBase + Offset);

	while (pID->Characteristics != 0 )
	{
		i = 0;
		pRealIAT = (PIMAGE_THUNK_DATA)((size_t)pBase + pID->FirstThunk);
		pOriginalIAT = (PIMAGE_THUNK_DATA)((size_t)pBase + pID->OriginalFirstThunk);
		//获取dll的名字;		
		dllName = (char*)((size_t)pBase + pID->Name);

		memset(buf,0,sizeof(buf));
		do 
		{
			if (i >= MAX_PATH)
			{
				break;
			}

			buf[i] = dllName[i];			
		} while (buf[i++]);	

		//hDll = GetModuleHandleW(buf);
		hDll = ::LoadLibraryW(buf);
		memset(buf,0,sizeof(buf));

		if ( !hDll)
		{
			return FALSE;
		}
		
		//获取DLL中每个导出函数的地址，填入IAT
		for (i = 0; pOriginalIAT[i].u1.Function ; i++)
		{
			if (pOriginalIAT[i].u1.Ordinal & IMAGE_ORDINAL_FLAG)  //这里的值给出的是导出序号;
			{
				lpFunction = ::GetProcAddress(hDll, (LPCSTR)(pOriginalIAT[i].u1.Ordinal & 0x0000FFFF));
			}
			else     //按照名字导入;
			{
				//获取此IAT项所描述的函数名称
				lpFunction = ::GetProcAddress(hDll,(char *)
					((PIMAGE_IMPORT_BY_NAME)((size_t)pBase + pOriginalIAT[i].u1.AddressOfData))->Name);
			}

			if (lpFunction)  //找到了;
			{
				pRealIAT[i].u1.Function = (size_t)lpFunction;//(PDWORD) lpFunction;			
			}
			else
			{
				return FALSE;
			}
		}

		//move to next
		pID ++;
	}

	return TRUE;
}

FARPROC CMemDll::GetProcAddress(LPCSTR lpProcName)
{
	if (NULL == m_pImageBase)
	{
		return NULL;
	}

	DWORD OffsetStart = 0;
	DWORD Size = 0;
	PIMAGE_EXPORT_DIRECTORY pExport = NULL;

	if (IMAGE_NT_OPTIONAL_HDR32_MAGIC == m_wOpMagic)
	{
		IMAGE_OPTIONAL_HEADER32* pOpHeader = (IMAGE_OPTIONAL_HEADER32*)m_pOpHeaderAddr;
		OffsetStart = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress ;
		Size = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
		pExport = (PIMAGE_EXPORT_DIRECTORY)
			((size_t)m_pImageBase + pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	}
	else
	{
		IMAGE_OPTIONAL_HEADER64* pOpHeader = (IMAGE_OPTIONAL_HEADER64*)m_pOpHeaderAddr;
		OffsetStart = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress ;
		Size = pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;
		pExport = (PIMAGE_EXPORT_DIRECTORY)
			((size_t)m_pImageBase + pOpHeader->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
	}

	if (OffsetStart == 0 ||	Size == 0)
	{
		return NULL;
	}	

	DWORD	dwBase = pExport->Base;
	DWORD	dwNumberOfFunctions = pExport->NumberOfFunctions;
	DWORD	dwNumberOfNames = pExport->NumberOfNames; //<= iNumberOfFunctions
	LPDWORD pAddressOfFunctions = (LPDWORD)((size_t)m_pImageBase + pExport->AddressOfFunctions );
	LPWORD  pAddressOfOrdinals = (LPWORD)((size_t)m_pImageBase + pExport->AddressOfNameOrdinals);
	LPDWORD pAddressOfNames  = (LPDWORD)((size_t)m_pImageBase + pExport->AddressOfNames);

	int iOrdinal = -1;

	if (((DWORD)lpProcName & 0xFFFF0000) == 0)  //IT IS A ORDINAL!
	{
		iOrdinal = (DWORD)lpProcName & 0x0000FFFF - dwBase;
	}
	else     //use name
	{
		DWORD dwIndex = 0;
		for (dwIndex = 0; dwIndex < dwNumberOfNames; dwIndex++)
		{
			char *pName = (char * )((size_t)m_pImageBase + pAddressOfNames[dwIndex]);
			if (strcmp(pName, lpProcName) == 0)
			{
				break;
			}
		}

		if (dwIndex < dwNumberOfNames)
		{
			iOrdinal = (DWORD)(pAddressOfOrdinals[dwIndex]);
		}
	}

	if (iOrdinal < 0 || iOrdinal >= dwNumberOfFunctions )
	{
		return NULL;
	}
	else
	{
		DWORD pFunctionOffset = pAddressOfFunctions[iOrdinal];
		if (pFunctionOffset > OffsetStart && pFunctionOffset < (OffsetStart + Size)) //maybe Export Forwarding
		{
			return NULL;
		}
		else
		{
			return (FARPROC)((size_t)m_pImageBase + pFunctionOffset);
		}
	}
}

void CMemDll::FreeLibrary()
{
	if (NULL == m_pImageBase || NULL == m_pfnDllMain)
	{
		return;
	}

	m_pfnDllMain((HINSTANCE)m_pImageBase, DLL_PROCESS_DETACH, 0);
	VirtualFree(m_pImageBase, 0, MEM_RELEASE);

	m_pImageBase	= NULL;
	m_pfnDllMain	= NULL;
	m_pDosHeader	= NULL;
	m_pNTHeader		= NULL;
	m_pOpHeaderAddr = NULL;
	m_pSectionHeader= NULL;	
	m_wOpMagic		= 0;
}
