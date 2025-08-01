export module PELoader:Loader;
import std;
import "sys_defs.h";
import :Parser;

namespace pe
{
	export struct SymbolData
	{
		std::uint64_t rvaLdrpHandleTlsData32 = 0; // LdrpHandleTlsData rva
		std::uint64_t rvaLdrpHandleTlsData64 = 0; // LdrpHandleTlsData rva
		std::uint64_t rvaLdrpInvertedFunctionTable32 = 0; // LdrpInvertedFunctionTable rva
		std::uint64_t rvaLdrpInvertedFunctionTable64 = 0; // LdrpInvertedFunctionTable rva
		std::uint64_t rvaRtlInsertInvertedFunctionTable32 = 0; // RtlInsertInvertedFunctionTable rva
		std::uint64_t rvaRtlInsertInvertedFunctionTable64 = 0; // RtlInsertInvertedFunctionTable rva
		std::uint64_t rvaLdrpReleaseTlsEntry32 = 0; // LdrpReleaseTlsEntry rva
		std::uint64_t rvaLdrpReleaseTlsEntry64 = 0; // LdrpReleaseTlsEntry rva
		// 32位未开启safeSEH,如果需要VEH时,可能需要LdrProtectMrdata函数的地址.
		// 暂时没这个需求
		// std::uint64_t rvaLdrProtectMrdata = 0; // LdrProtectMrdata rva
	};

	export SymbolData g_symbols;

	constexpr size_t align_value_up(size_t value, size_t alignment)
	{
		return (value + alignment - 1) & ~(alignment - 1);
	}

	export class MemoryModule
	{
	public:
		explicit MemoryModule(const Parser<parser_flag::HasFileAligned>& parser)
		{
			const DWORD sizeOfImage = parser.getSizeOfImage();
			m_dataGuard.allocateData(sizeOfImage);

			// 拷贝所有头
			const DWORD sizeOfHeaders = parser.getSizeOfHeaders();
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

			m_dataParser = Parser<parser_flag::HasSectionAligned>(m_dataGuard.pData);
		}

		explicit MemoryModule(Parser<parser_flag::HasSectionAligned> parser)
		{
			m_dataParser = std::move(parser);
		}

		MemoryModule(const MemoryModule&) = delete;
		MemoryModule(MemoryModule&&) = delete;
		MemoryModule& operator=(const MemoryModule&) = delete;
		MemoryModule& operator=(MemoryModule&&) = delete;

		const char* getBaseAddr() const { return m_dataParser.getBaseAddr(); }
		std::uint32_t getSizeOfImage() const { return m_dataParser.getSizeOfImage(); }

		const Parser<parser_flag::HasSectionAligned>& getParser() const { return m_dataParser; }

	public:
		void setSectionProtection() const
		{
			char* pBase = const_cast<char*>(m_dataParser.getBaseAddr());
			PIMAGE_NT_HEADERS pNTHeader = m_dataParser.getNtHeader<PIMAGE_NT_HEADERS>();
			IMAGE_SECTION_HEADER* pSectionHeader = IMAGE_FIRST_SECTION(pNTHeader);
			// 设置各个节的可读、可写、可执行属性
			for (DWORD i = 0; i < pNTHeader->FileHeader.NumberOfSections; ++i, ++pSectionHeader)
			{
				const LPVOID address = pBase + pSectionHeader->VirtualAddress;
				const SIZE_T size = align_value_up(pSectionHeader->Misc.VirtualSize, pNTHeader->OptionalHeader.SectionAlignment);
				DWORD dwOldProtect = 0;
				DWORD dwNewProtect = PAGE_NOACCESS;
				// executable
				if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_EXECUTE) != 0)
				{
					// writeable
					if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) != 0)
					{
						dwNewProtect = PAGE_EXECUTE_READWRITE;
					}
					// readable
					else if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_READ) != 0)
					{
						dwNewProtect = PAGE_EXECUTE_READ;
					}
					else
					{
						dwNewProtect = PAGE_EXECUTE;
					}
				}
				else
				{
					// writeable
					if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_WRITE) != 0)
					{
						dwNewProtect = PAGE_READWRITE;
					}
					// readable
					else if ((pSectionHeader->Characteristics & IMAGE_SCN_MEM_READ) != 0)
					{
						dwNewProtect = PAGE_READONLY;
					}
				}
				if (pSectionHeader->Characteristics & IMAGE_SCN_MEM_NOT_CACHED)
				{
					dwNewProtect |= PAGE_NOCACHE;
				}

				VirtualProtect(address, size, dwNewProtect, &dwOldProtect);
			}
		}

	private:
		class DataGuard
		{
		public:
			char* pData{nullptr};


			void allocateData(std::uint32_t size)
			{
				pData = static_cast<char*>(VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
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
		Parser<parser_flag::HasSectionAligned> m_dataParser;
	};

	export std::unique_ptr<MemoryModule> create_module_from_file_memory(const void* fileMemory)
	{
		return std::make_unique<MemoryModule>(Parser{static_cast<const char*>(fileMemory)});
	}

	export std::unique_ptr<MemoryModule> create_module_from_mapped_memory(const void* mappedMemory)
	{
		return std::make_unique<MemoryModule>(Parser<parser_flag::HasSectionAligned>{static_cast<const char*>(mappedMemory)});
	}

	export BOOL flush_instruction_cache()
	{
		return FlushInstructionCache(reinterpret_cast<HANDLE>(-1), nullptr, 0);
	}
}
