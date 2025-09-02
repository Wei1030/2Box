module;
#include <Wbemidl.h>
#pragma comment (lib, "wbemuuid.lib")
export module Hook:Ole32;

import "sys_defs.h";
import :Core;

namespace hook
{
	template <auto Trampoline>
	HRESULT STDAPICALLTYPE CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID FAR* ppv)
	{
		if (memcmp(&rclsid, &CLSID_WbemLocator, sizeof(CLSID)) == 0)
		{
			return REGDB_E_CLASSNOTREG;
		}
		return Trampoline(rclsid, pUnkOuter, dwClsContext, riid, ppv);
	}

	template <auto Trampoline>
	HRESULT STDAPICALLTYPE CoCreateInstanceEx(REFCLSID Clsid, IUnknown* punkOuter, DWORD dwClsCtx, COSERVERINFO* pServerInfo, DWORD dwCount, MULTI_QI* pResults)
	{
		if (memcmp(&Clsid, &CLSID_WbemLocator, sizeof(CLSID)) == 0)
		{
			if (pResults)
			{
				pResults->hr = REGDB_E_CLASSNOTREG;
			}
			return REGDB_E_CLASSNOTREG;
		}
		return Trampoline(Clsid, punkOuter, dwClsCtx, pServerInfo, dwCount, pResults);
	}

	void hook_ole32()
	{
		create_hook_by_func_ptr<&::CoCreateInstance>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&CoCreateInstance<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::CoCreateInstanceEx>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&CoCreateInstanceEx<trampolineConst.value>};
		});
	}
}
