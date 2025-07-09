export module Utility.SystemInfo;

import std;
import "sys_defs.h";

namespace sys_info
{
	export std::wstring get_system_dir()
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

	export std::wstring get_system_wow64_dir()
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
