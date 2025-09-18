export module Utility.SystemInfo;

import std;
import "sys_defs.h";

namespace sys_info
{
	namespace detail
	{
		std::wstring get_system_dir()
		{
			UINT systemDirLength = GetSystemDirectoryW(nullptr, 0);
			if (systemDirLength == 0)
			{
				throw std::runtime_error(std::format("GetSystemDirectoryW failed, error code: {}", GetLastError()));
			}
			std::wstring systemDir;
			systemDir.resize(systemDirLength);
			systemDirLength = GetSystemDirectoryW(systemDir.data(), systemDirLength);
			if (systemDirLength == 0)
			{
				throw std::runtime_error(std::format("GetSystemDirectoryW failed, error code: {}", GetLastError()));
			}
			systemDir.resize(systemDirLength);
			return systemDir;
		}

		std::wstring get_system_wow64_dir()
		{
			UINT systemDirLength = GetSystemWow64DirectoryW(nullptr, 0);
			if (systemDirLength == 0)
			{
				throw std::runtime_error(std::format("GetSystemWow64DirectoryW failed, error code: {}", GetLastError()));
			}
			std::wstring systemDir;
			systemDir.resize(systemDirLength);
			systemDirLength = GetSystemWow64DirectoryW(systemDir.data(), systemDirLength);
			if (systemDirLength == 0)
			{
				throw std::runtime_error(std::format("GetSystemWow64DirectoryW failed, error code: {}", GetLastError()));
			}
			systemDir.resize(systemDirLength);
			return systemDir;
		}
	}

	// 暂不支持32位进程拿64位系统目录，没这个需求。
	// 有需求再说。TODO: 如果要拿，则需要用到 Wow64DisableWow64FsRedirection / Wow64RevertWow64FsRedirection
	export template <ArchBit BitType = CURRENT_ARCH_BIT>
	std::wstring get_system_dir()
	{
		if constexpr (BitType == ArchBit::Bit32)
		{
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				return detail::get_system_wow64_dir();
			}
			else
			{
				return detail::get_system_dir();
			}
		}
		else
		{
			return detail::get_system_dir();
		}
	}

	namespace detail
	{
		class NonCopyOrMovable
		{
		public:
			NonCopyOrMovable() = default;

			NonCopyOrMovable(const NonCopyOrMovable&) = delete;
			NonCopyOrMovable& operator=(const NonCopyOrMovable&) = delete;
			NonCopyOrMovable(NonCopyOrMovable&&) = delete;
			NonCopyOrMovable& operator=(NonCopyOrMovable&&) = delete;
		};

		class SysFileWrapper : public NonCopyOrMovable
		{
		public:
			explicit SysFileWrapper(std::wstring_view filePath)
			{
				m_hFile = CreateFileW(filePath.data(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
				                      nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
				if (m_hFile == INVALID_HANDLE_VALUE)
				{
					throw std::runtime_error(std::format("CreateFileW failed error code: {}", GetLastError()));
				}
			}

			~SysFileWrapper()
			{
				if (m_hFile != INVALID_HANDLE_VALUE)
				{
					CloseHandle(m_hFile);
				}
			}

			HANDLE handle() const
			{
				return m_hFile;
			}

		private:
			HANDLE m_hFile{INVALID_HANDLE_VALUE};
		};

		class SysImageMappingWrapper : public NonCopyOrMovable
		{
		public:
			explicit SysImageMappingWrapper(const SysFileWrapper& file)
			{
				m_hFileMapping = CreateFileMappingW(file.handle(), nullptr, PAGE_READONLY | SEC_IMAGE, 0, 0, nullptr);
				if (m_hFileMapping == nullptr)
				{
					throw std::runtime_error{std::format("CreateFileMappingW failed, error code: {}", GetLastError())};
				}
			}

			~SysImageMappingWrapper()
			{
				if (m_hFileMapping)
				{
					CloseHandle(m_hFileMapping);
				}
			}

			HANDLE handle() const
			{
				return m_hFileMapping;
			}

		private:
			HANDLE m_hFileMapping{nullptr};
		};
	}

	export class SysDllMapHelper : public detail::NonCopyOrMovable
	{
	public:
		explicit SysDllMapHelper(std::wstring_view dllPath) : m_file{dllPath}
		{
			m_pMemory = MapViewOfFile(m_mapping.handle(), FILE_MAP_READ, 0, 0, 0);
			if (m_pMemory == nullptr)
			{
				throw std::runtime_error{std::format("MapViewOfFile failed, error code: {}", GetLastError())};
			}
		}

		~SysDllMapHelper()
		{
			if (m_pMemory)
			{
				UnmapViewOfFile(m_pMemory);
			}
		}

		void* memAddress() const
		{
			return m_pMemory;
		}

	private:
		detail::SysFileWrapper m_file;
		detail::SysImageMappingWrapper m_mapping{m_file};
		void* m_pMemory;
	};

	export
	template <ArchBit BitType = CURRENT_ARCH_BIT>
	SysDllMapHelper get_sys_dll_mapped(std::wstring_view dllName)
	{
		namespace fs = std::filesystem;
		const fs::path dllPath{fs::weakly_canonical(fs::path{get_system_dir<BitType>()} / fs::path{dllName})};
		return SysDllMapHelper{dllPath.native()};
	}

	export
	template <ArchBit BitType = CURRENT_ARCH_BIT>
	SysDllMapHelper get_ntdll_mapped()
	{
		return get_sys_dll_mapped<BitType>(L"ntdll.dll");
	}

	export
	template <ArchBit BitType = CURRENT_ARCH_BIT>
	SysDllMapHelper get_kernel32_mapped()
	{
		return get_sys_dll_mapped<BitType>(L"kernel32.dll");
	}
}
