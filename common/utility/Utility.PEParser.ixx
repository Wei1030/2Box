export module Utility.PEParser;

import std;
import "sys_defs.h";

namespace utils
{
	/*
	 *
	 // 文件头
	 typedef struct _IMAGE_FILE_HEADER {
		...
		WORD    NumberOfSections;				// 区段数量
		...
		WORD    SizeOfOptionalHeader;			// 可选头大小	
		WORD    Characteristics;				// 文件属性
	} IMAGE_FILE_HEADER;

	// 可选头
	typedef struct _IMAGE_OPTIONAL_HEADER64 {
		WORD        Magic;						// 标志32位还是64位
		...
		DWORD       AddressOfEntryPoint;		// 入口点
		....
		ULONGLONG   ImageBase;					// 建议的内存基址
		DWORD       SectionAlignment;			// 内存对齐大小
		DWORD       FileAlignment;				// 文件对齐大小
		...
		DWORD       SizeOfImage;				// 文件在内存中按SectionAlignment对齐后的大小
		DWORD       SizeOfHeaders;				// 所有头部加起来按FileAlignment对齐后的大小
		DWORD       CheckSum;
		...
		DWORD       NumberOfRvaAndSizes;		// 数据目录表个数
		IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES]; // 数据目录表
	} IMAGE_OPTIONAL_HEADER64;

	// 区段头
	typedef struct _IMAGE_SECTION_HEADER {
		BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];	// 区段名称，注意不会以0结尾
		union {
				DWORD   PhysicalAddress;
				DWORD   VirtualSize;
		} Misc;									// 该区段在内存中的真实大小(未对齐)
		DWORD   VirtualAddress;					// 该区段在内存中的偏移( RVA )
		DWORD   SizeOfRawData;					// 该区段在文件中对齐后的大小
		DWORD   PointerToRawData;				// 该区段在文件中的偏移( FOA )
		...
		DWORD   Characteristics;				// 该区段属性(可读、可写、可执行等)
	} IMAGE_SECTION_HEADER;

	// 数据表目录:
	IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
	IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
	...
	typedef struct _IMAGE_DATA_DIRECTORY {
		DWORD   VirtualAddress;					// RVA
		DWORD   Size;							// 大小，可以被篡改，不关心
	} IMAGE_DATA_DIRECTORY;

	*/
	export class PEParser
	{
	public:
		struct CommonImageNtHeaders
		{
			DWORD signature;
			IMAGE_FILE_HEADER fileHeader;
		};

		PEParser(const char* sRawDll, std::uint32_t dwSize)
		{
			//检查长度;
			if (dwSize < sizeof(IMAGE_DOS_HEADER))
			{
				throw std::runtime_error("PEParser::file size is too small");
			}

			m_pDosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(sRawDll);

			//检查dos头的标记;
			if (m_pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			{
				throw std::runtime_error("PEParser::file header signature is not correct. it should be MZ");
			}
			const std::uint32_t opHeaderPos = m_pDosHeader->e_lfanew + sizeof(CommonImageNtHeaders);
			if (dwSize < opHeaderPos)
			{
				throw std::runtime_error("PEParser::file size is too small");
			}

			m_pNtHeader = sRawDll + m_pDosHeader->e_lfanew;
			m_optionalHeaderMagic = *reinterpret_cast<const WORD*>(m_pNtHeader + sizeof(CommonImageNtHeaders));

			processOnNtHeader([this, dwSize, opHeaderPos](const auto& ntHeader)-> void
			{
				if (dwSize < m_pDosHeader->e_lfanew + sizeof(ntHeader)
					|| dwSize < opHeaderPos + ntHeader.FileHeader.SizeOfOptionalHeader)
				{
					throw std::runtime_error("PEParser::file size is too small");
				}

				m_pOpHeaderAddr = reinterpret_cast<const char*>(&ntHeader.OptionalHeader);
				m_pSectionHeader = reinterpret_cast<const IMAGE_SECTION_HEADER*>(m_pOpHeaderAddr + ntHeader.FileHeader.SizeOfOptionalHeader);
				m_numberOfSections = ntHeader.FileHeader.NumberOfSections;
			});
		}

		template <typename Func>
			requires std::predicate<Func, const char*>
		DWORD getProcFOA(const Func& compareFunc) const
		{
			static auto [pExportDir, pdwNames, pdwNameOrdinals, pdwFunctionAddresses] = getExportDirInfo();

			const char* pBase = reinterpret_cast<const char*>(m_pDosHeader);

			// Walk the array of this module's function names 
			for (DWORD n = 0; n < pExportDir->NumberOfNames; n++)
			{
				// Get the function name
				const char* szFuncName = pBase + rvaToFoa(pdwNames[n]);
				if (!compareFunc(szFuncName))
				{
					continue;
				}

				// We found the specified function
				// --> Get this function's ordinal value
				const WORD ordinal = pdwNameOrdinals[n];
				// Get the address of this function's address
				return rvaToFoa(pdwFunctionAddresses[ordinal]);
			}

			throw std::runtime_error("PEParser::getProcAddressOffset: failed to find exported function");
		}

		decltype(auto) processOnNtHeader(const auto& func) const
		{
			if (m_optionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			{
				return func(*reinterpret_cast<const IMAGE_NT_HEADERS32*>(m_pNtHeader));
			}
			if (m_optionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			{
				return func(*reinterpret_cast<const IMAGE_NT_HEADERS64*>(m_pNtHeader));
			}
			throw std::runtime_error("PEParser: unsupported image file header");
		}

		decltype(auto) processOnOpHeader(const auto& func) const
		{
			if (m_optionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR32_MAGIC)
			{
				return func(*reinterpret_cast<const IMAGE_OPTIONAL_HEADER32*>(m_pOpHeaderAddr));
			}
			if (m_optionalHeaderMagic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
			{
				return func(*reinterpret_cast<const IMAGE_OPTIONAL_HEADER64*>(m_pOpHeaderAddr));
			}
			throw std::runtime_error("PEParser: unsupported image file header");
		}

	private:
		DWORD rvaToFoa(DWORD dwRva) const
		{
			if (dwRva < m_pSectionHeader[0].VirtualAddress)
			{
				return dwRva;
			}
			
			for (WORD wIndex = 0; wIndex < m_numberOfSections; wIndex++)
			{
				const IMAGE_SECTION_HEADER& currentSection = m_pSectionHeader[wIndex];
				if (dwRva >= currentSection.VirtualAddress && dwRva < currentSection.VirtualAddress + currentSection.Misc.VirtualSize)
				{
					// 从该区段开始的距离 = RVA - 区段RVA = FOA - 区段FOA
					// 所以 FOA = RVA - 区段RVA + 区段FOA
					return dwRva - currentSection.VirtualAddress + currentSection.PointerToRawData;
				}
			}
			throw std::runtime_error("PEParser: rvaToFoa failed to find section header");
		}

		auto getExportDirInfo() const
		{
			const IMAGE_DATA_DIRECTORY& directoryEntry = processOnOpHeader([](const auto& opHeader)-> const IMAGE_DATA_DIRECTORY&
			{
				return opHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			});
			if (directoryEntry.Size == 0 || directoryEntry.VirtualAddress == 0)
			{
				throw std::runtime_error("getExportDirInfo fail");
			}
			struct ExportDirInfo
			{
				const IMAGE_EXPORT_DIRECTORY* pExportDir;
				const DWORD* pdwNames;
				const WORD* pdwNameOrdinals;
				const DWORD* pdwFunctionAddresses;
			};
			ExportDirInfo result;
			const char* pBase = reinterpret_cast<const char*>(m_pDosHeader);
			result.pExportDir = reinterpret_cast<const IMAGE_EXPORT_DIRECTORY*>(pBase + rvaToFoa(directoryEntry.VirtualAddress));
			result.pdwNames = reinterpret_cast<const DWORD*>(pBase + rvaToFoa(result.pExportDir->AddressOfNames));
			result.pdwNameOrdinals = reinterpret_cast<const WORD*>(pBase + rvaToFoa(result.pExportDir->AddressOfNameOrdinals));
			result.pdwFunctionAddresses = reinterpret_cast<const DWORD*>(pBase + rvaToFoa(result.pExportDir->AddressOfFunctions));
			return result;
		}

	private:
		const IMAGE_DOS_HEADER* m_pDosHeader{nullptr};
		const char* m_pNtHeader{nullptr};
		WORD m_optionalHeaderMagic{0};
		const char* m_pOpHeaderAddr{nullptr};
		const IMAGE_SECTION_HEADER* m_pSectionHeader{nullptr};
		WORD m_numberOfSections{0};
	};
}
