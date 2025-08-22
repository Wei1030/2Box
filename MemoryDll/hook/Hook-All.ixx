export module Hook:All;

import :Core;
import :Ntdll;
import :Kernel32;
import :Advapi32;

namespace hook
{
	export void hook_all()
	{
		hook_ntdll();
		hook_kernel32();
		hook_advapi32();
		
		HookManager::instance().installAll();
	}
}
