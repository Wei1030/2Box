export module Hook:Advapi32;

import "sys_defs.h";
import std;
import :Core;
import GlobalData;
import PELoader;

namespace hook
{
	LSTATUS APIENTRY RegLoadAppKeyW(_In_ LPCWSTR lpFile, _Out_ PHKEY phkResult, _In_ REGSAM samDesired, _In_ DWORD dwOptions, _Reserved_ DWORD Reserved)
	{
		namespace fs = std::filesystem;
		return RegCreateKeyExW(global::Data::get().appKey(),
		                       std::format(L"{}_{}", fs::path{lpFile}.stem().native(), global::Data::get().envFlagName()).c_str(),
		                       Reserved, nullptr, dwOptions, samDesired, nullptr, phkResult, nullptr);
	}

	void hook_advapi32()
	{
		// win7 or earlier
		if (!pe::g_os_version.isWindows8OrGreater)
		{
			create_hook_by_func_ptr<&::RegLoadAppKeyW>().setHook(&RegLoadAppKeyW);
		}
	}
}
