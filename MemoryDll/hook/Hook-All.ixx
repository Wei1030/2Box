export module Hook:All;

import :Core;
import :Ntdll;

namespace hook
{
	export void hook_all()
	{
		hook_ntdll();
		
		HookManager::instance().installAll();
	}
}
