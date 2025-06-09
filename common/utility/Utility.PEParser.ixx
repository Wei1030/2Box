export module Utility.PEParser;

import std;
import "sys_defs.h";

namespace utils
{
	static constexpr std::uint32_t SIGNATURE_AND_FILE_HEADER_SIZE = sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER);

	namespace pe_parser_flag
	{
		export struct HasFileAligned
		{
		};

		export struct HasSectionAligned
		{
		};
	}

	export
	template <typename FlagType = pe_parser_flag::HasFileAligned>
	class PEParser
	{
	public:
		PEParser() = default;

		explicit PEParser(const char* baseAddr)
		{
			if (baseAddr == nullptr)
			{
				throw std::invalid_argument("baseAddr cannot be null");
			}

			m_pDosHeader = reinterpret_cast<const IMAGE_DOS_HEADER*>(baseAddr);

			//检查dos头的标记;
			if (m_pDosHeader->e_magic != IMAGE_DOS_SIGNATURE)
			{
				throw std::runtime_error("PEParser::file header signature is not correct. it should be MZ");
			}

			m_pNtHeader = baseAddr + m_pDosHeader->e_lfanew;
			m_optionalHeaderMagic = *reinterpret_cast<const WORD*>(m_pNtHeader + SIGNATURE_AND_FILE_HEADER_SIZE);

			processOnNtHeader([this](const auto& ntHeader)-> void
			{
				m_pOpHeaderAddr = reinterpret_cast<const char*>(&ntHeader.OptionalHeader);
				m_pSectionHeader = reinterpret_cast<const IMAGE_SECTION_HEADER*>(m_pOpHeaderAddr + ntHeader.FileHeader.SizeOfOptionalHeader);
				m_numberOfSections = ntHeader.FileHeader.NumberOfSections;
			});
		}

		const char* getBaseAddr() const noexcept { return reinterpret_cast<const char*>(m_pDosHeader); }
		const IMAGE_DOS_HEADER* getDosHeader() const noexcept { return m_pDosHeader; }
		const IMAGE_SECTION_HEADER* getSectionHeader() const noexcept { return m_pSectionHeader; }
		int getNumberOfSections() const noexcept { return m_numberOfSections; }

		DWORD getProcRVA(std::string_view procName) const
		{
			const auto& [pExportDir, pdwNames, pdwNameOrdinals, pdwFunctionAddresses] = getExportDirInfo();

			// Walk the array of this module's function names 
			for (DWORD n = 0; n < pExportDir->NumberOfNames; n++)
			{
				// Get the function name
				const char* szFuncName = getDataByRva<const char*>(pdwNames[n]);
				if (procName != szFuncName)
				{
					continue;
				}

				// We found the specified function
				// --> Get this function's ordinal value
				const WORD ordinal = pdwNameOrdinals[n];
				// Get this function's RVA
				return pdwFunctionAddresses[ordinal];
			}

			return 0;
		}

		DWORD getProcFOA(std::string_view procName) const
		{
			return rvaToFoa(getProcRVA(procName));
		}

		template <typename ReturnType>
		ReturnType getDataByRva(DWORD dwRva) const
		{
			DWORD realOffset = dwRva;
			if constexpr (std::is_same_v<FlagType, pe_parser_flag::HasFileAligned>)
			{
				realOffset = rvaToFoa(dwRva);
			}
			const char* pBase = reinterpret_cast<const char*>(m_pDosHeader);
			return reinterpret_cast<ReturnType>(pBase + realOffset);
		}

		decltype(auto) processOnNtHeader(const auto& func) const
		{
			if (!m_pNtHeader)
			{
				throw std::runtime_error("PEParser::processOnNtHeader: failed to find NtHeader");
			}
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
			if (!m_pOpHeaderAddr)
			{
				throw std::runtime_error("PEParser::processOnOpHeader: failed to find optional header");
			}
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

		struct ExportDirInfo
		{
			// typedef struct _IMAGE_EXPORT_DIRECTORY {
			//  ...
			// 	DWORD   Name;	// 文件名
			// 	DWORD   Base;	// 导出函数起始序号
			// 	DWORD   NumberOfFunctions;	// 导出函数个数 序号最大的-序号最小的+1
			// 	DWORD   NumberOfNames;	// 以名称导出的函数的个数
			// 	DWORD   AddressOfFunctions;     // RVA from base of image
			// 	DWORD   AddressOfNames;         // RVA from base of image
			// 	DWORD   AddressOfNameOrdinals;  // RVA from base of image
			// } IMAGE_EXPORT_DIRECTORY;
			// 遍历时，以NumberOfNames为遍历次数比较方便，其index直接与NameOrdinals一一对应， NameOrdinals中的值又直接对应AddressOfFunctions的下标
			const IMAGE_EXPORT_DIRECTORY* pExportDir{nullptr};
			const DWORD* pdwNames{nullptr};
			const WORD* pdwNameOrdinals{nullptr};
			const DWORD* pdwFunctionAddresses{nullptr};
		};

		const ExportDirInfo& getExportDirInfo() const
		{
			if (!m_exportDirInfo.pExportDir)
			{
				const_cast<PEParser*>(this)->initExportDirInfo();
			}
			return m_exportDirInfo;
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

		void initExportDirInfo()
		{
			const IMAGE_DATA_DIRECTORY* directoryEntry = processOnOpHeader([](const auto& opHeader)
			{
				return &opHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
			});
			if (!directoryEntry || directoryEntry->VirtualAddress == 0)
			{
				throw std::runtime_error("getExportDirInfo fail");
			}
			m_exportDirInfo.pExportDir = getDataByRva<const IMAGE_EXPORT_DIRECTORY*>(directoryEntry->VirtualAddress);
			m_exportDirInfo.pdwNames = getDataByRva<const DWORD*>(m_exportDirInfo.pExportDir->AddressOfNames);
			m_exportDirInfo.pdwNameOrdinals = getDataByRva<const WORD*>(m_exportDirInfo.pExportDir->AddressOfNameOrdinals);
			m_exportDirInfo.pdwFunctionAddresses = getDataByRva<const DWORD*>(m_exportDirInfo.pExportDir->AddressOfFunctions);
		}

	private:
		const IMAGE_DOS_HEADER* m_pDosHeader{nullptr};

		// typedef struct _IMAGE_NT_HEADERS64
		// {
		// 	DWORD Signature;
		// 	IMAGE_FILE_HEADER FileHeader;
		// 	...
		// } IMAGE_NT_HEADERS64;
		// 文件头
		// typedef struct _IMAGE_FILE_HEADER {
		// 	...
		// 	WORD    NumberOfSections;	// 区段数量
		// 	...
		// 	WORD    SizeOfOptionalHeader;	// 可选头大小	
		// 	WORD    Characteristics;	// 文件属性
		// } IMAGE_FILE_HEADER;
		const char* m_pNtHeader{nullptr};

		WORD m_optionalHeaderMagic{0};
		// 可选头
		// typedef struct _IMAGE_OPTIONAL_HEADER64 {
		// 	WORD        Magic;	// 标志32位还是64位
		// 	...
		// 	DWORD       AddressOfEntryPoint;// 入口点
		// 	....
		// 	ULONGLONG   ImageBase;	// 建议的内存基址
		// 	DWORD       SectionAlignment;// 内存对齐大小
		// 	DWORD       FileAlignment;	// 文件对齐大小
		// 	...
		// 	DWORD       SizeOfImage;	// 文件在内存中按SectionAlignment对齐后的大小
		// 	DWORD       SizeOfHeaders;	// 所有头部加起来按FileAlignment对齐后的大小
		// 	...
		// 	DWORD       NumberOfRvaAndSizes;// 数据目录表个数
		// 	IMAGE_DATA_DIRECTORY DataDirectory[IMAGE_NUMBEROF_DIRECTORY_ENTRIES]; // 数据目录表
		// } IMAGE_OPTIONAL_HEADER64;
		const char* m_pOpHeaderAddr{nullptr};

		// 区段头
		// typedef struct _IMAGE_SECTION_HEADER {
		// 	BYTE    Name[IMAGE_SIZEOF_SHORT_NAME];	// 区段名称，注意不会以0结尾
		// 	union {
		// 		DWORD   PhysicalAddress;
		// 		DWORD   VirtualSize;
		// 	} Misc;	// 该区段在内存中的真实大小(未对齐)
		// 	DWORD   VirtualAddress;	// 该区段在内存中的偏移( RVA )
		// 	DWORD   SizeOfRawData;	// 该区段在文件中对齐后的大小
		// 	DWORD   PointerToRawData;	// 该区段在文件中的偏移( FOA )
		// 	...
		// 	DWORD   Characteristics;	// 该区段属性(可读、可写、可执行等)
		// } IMAGE_SECTION_HEADER;
		const IMAGE_SECTION_HEADER* m_pSectionHeader{nullptr};
		WORD m_numberOfSections{0};

		// 数据表目录:
		// IMAGE_DIRECTORY_ENTRY_EXPORT          0   // Export Directory
		// IMAGE_DIRECTORY_ENTRY_IMPORT          1   // Import Directory
		// ...
		// typedef struct _IMAGE_DATA_DIRECTORY {
		// 	DWORD   VirtualAddress;	// RVA
		// 	DWORD   Size;	// 大小，可以被篡改，不关心
		// } IMAGE_DATA_DIRECTORY;

		// 导出表
		ExportDirInfo m_exportDirInfo;
	};

	export class PEAlignedImage
	{
	public:
		explicit PEAlignedImage(const PEParser<pe_parser_flag::HasFileAligned>& parser)
		{
			struct ReturnType
			{
				DWORD sizeOfImage;
				DWORD sizeOfHeaders;
			};
			const auto [sizeOfImage, sizeOfHeaders] = parser.processOnOpHeader([](const auto& opHeader)
			{
				return ReturnType{opHeader.SizeOfImage, opHeader.SizeOfHeaders};
			});
			m_dataGuard.allocateData(sizeOfImage);

			// 拷贝所有头
			m_dataGuard.writeData(0, parser.getBaseAddr(), sizeOfHeaders);

			// 拷贝所有节
			for (int i = 0; i < parser.getNumberOfSections(); ++i)
			{
				const IMAGE_SECTION_HEADER& currentSection = parser.getSectionHeader()[i];
				if (currentSection.VirtualAddress == 0 || currentSection.SizeOfRawData == 0)
				{
					continue;
				}
				const void* pSrc = parser.getBaseAddr() + currentSection.PointerToRawData;
				m_dataGuard.writeData(currentSection.VirtualAddress, pSrc, currentSection.SizeOfRawData);
			}

			m_dataParser = PEParser<pe_parser_flag::HasSectionAligned>(m_dataGuard.pData);
		}

		PEAlignedImage(const PEAlignedImage&) = delete;
		PEAlignedImage(PEAlignedImage&&) = delete;
		PEAlignedImage& operator=(const PEAlignedImage&) = delete;
		PEAlignedImage& operator=(PEAlignedImage&&) = delete;

		const char* data() const { return m_dataGuard.pData; }
		std::uint32_t size() const { return m_dataGuard.nSize; }

		const PEParser<pe_parser_flag::HasSectionAligned>& getParser() const { return m_dataParser; }

	private:
		class DataGuard
		{
		public:
			char* pData{nullptr};
			std::uint32_t nSize{0};

			void allocateData(std::uint32_t size)
			{
				pData = static_cast<char*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
				nSize = size;
			}

			~DataGuard()
			{
				if (pData)
				{
					VirtualFree(pData, 0, MEM_RELEASE);
				}
			}

			void writeData(std::uint32_t dstOffset, void const* src, std::uint32_t size)
			{
				std::memcpy(pData + dstOffset, src, size);
			}
		};

		DataGuard m_dataGuard;
		PEParser<pe_parser_flag::HasSectionAligned> m_dataParser;
	};
}
