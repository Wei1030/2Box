export module Hook:All;

import :Core;
import :Ntdll;
import :Kernel32;
import :Advapi32;
import :User32;
import :Ole32;
import :Iphlpapi;
import :Netapi32;

namespace hook
{
	export void hook_all()
	{
		hook_ntdll();
		hook_kernel32();
		hook_advapi32();
		hook_user32();
		hook_ole32();
		hook_iphlpapi();
		hook_netapi32();
		
		HookManager::instance().installAll();
	}
}
