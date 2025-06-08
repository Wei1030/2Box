export module Utility.MemDll;

import std;
import "sys_defs.h";
import Utility.PEParser;

namespace utils
{
	struct MemDllDefaultOperator
	{
		static void* allocate(size_t size)
		{
			return std::malloc(size);
		}

		static void deallocate(void* ptr)
		{
			std::free(ptr);
		}

		static void* copy(void* dst, void const* src, size_t size)
		{
			return std::memcpy(dst, src, size);
		}
	};

	export
	template <typename MemOperator>
		requires requires(MemOperator memOperator)
		{
			{ memOperator.allocate(size_t{}) } -> std::same_as<void*>;
			{ memOperator.deallocate(nullptr) } -> std::same_as<void>;
			{ memOperator.copy(nullptr, nullptr, size_t{}) } -> std::same_as<void*>;
			requires std::is_move_constructible_v<MemOperator>;
			requires std::is_move_assignable_v<MemOperator>;
		}
	class MemDll
	{
	public:
		MemDll(const char* sRawDll, std::uint32_t dwSize, MemOperator&& memOperator = MemDllDefaultOperator{})
			: m_dataGuard(std::move(memOperator))
		{
			PEParser parserNotAligned(sRawDll, dwSize);
			struct ReturnType
			{
				DWORD sizeOfImage;
				DWORD sizeOfHeaders;
			};
			const auto [sizeOfImage, sizeOfHeaders] = parserNotAligned.processOnOpHeader([](const auto& opHeader)
			{
				return ReturnType{opHeader.SizeOfImage, opHeader.SizeOfHeaders};
			});
			m_dataGuard.allocateData(sizeOfImage);

			// 拷贝所有头
			m_dataGuard.writeData(0, sRawDll, sizeOfHeaders);

			// 拷贝所有节
			for (int i = 0; i < parserNotAligned.getNumberOfSections(); ++i)
			{
				const IMAGE_SECTION_HEADER& currentSection = parserNotAligned.getSectionHeader()[i];
				if (currentSection.VirtualAddress == 0 || currentSection.SizeOfRawData == 0)
				{
					continue;
				}
				const void* pSrc = parserNotAligned.getBaseAddr() + currentSection.PointerToRawData;
				m_dataGuard.writeData(currentSection.VirtualAddress, pSrc, currentSection.SizeOfRawData);
			}
		}

		char* data() { return m_dataGuard.pData; }
		std::uint32_t size() const { return m_dataGuard.nSize; }
		MemOperator& getMemOperator() { return m_dataGuard.memOperator; }

	private:
		// MemDll构造中可能会在创建pData后抛出异常，MemDll析构就不会被调用
		// 需要再用个成员保护一下
		struct DataGuard
		{
			char* pData{nullptr};
			std::uint32_t nSize{0};
			MemOperator memOperator;

			explicit DataGuard(MemOperator&& o): memOperator(std::move(o))
			{
			}

			~DataGuard()
			{
				if (pData)
				{
					memOperator.deallocate(pData);
				}
			}

			void allocateData(std::uint32_t size)
			{
				pData = static_cast<char*>(memOperator.allocate(size));
				nSize = size;
			}

			void writeData(std::uint32_t dstOffset, void const* src, std::uint32_t size)
			{
				memOperator.copy(pData + dstOffset, src, size);
			}
		};

		DataGuard m_dataGuard;
	};
}
