#ifdef REFLECTIVE_INJECT
#ifndef _WIN64
#pragma comment(linker, "/EXPORT:initialize=_initialize@4")
#endif
#endif 

import "sys_defs.h";
import std;
import PELoader;

#include "biz_initializer.h"

#ifdef REFLECTIVE_INJECT
extern "C" __declspec(dllexport) unsigned long __stdcall initialize(void* lpThreadParameter)
{
	if (!lpThreadParameter)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}

	const ReflectiveInjectParams& injectParams = *static_cast<ReflectiveInjectParams*>(lpThreadParameter);
	const EssentialData& essentialData = injectParams.essentialData;
	const char* pThisModuleAddress = reinterpret_cast<const char*>(injectParams.injectionInfo.dllAddress);
	const pe::MemoryModule thisModule{pe::Parser<pe::parser_flag::HasSectionAligned>{pThisModuleAddress}};

	pe::fill_os_version(essentialData.version);
	pe::fill_all_symbols(essentialData.symRva32, essentialData.symRva64);

	pe::set_section_protection(thisModule);
	pe::enable_exceptions(thisModule);
	if (!pe::init_static_tls(thisModule))
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
	pe::wipe_header_memory(thisModule);

	biz_initialize(injectParams.envFlag, injectParams.envPath, injectParams.envPathCount);
	return 0;
}
#endif 

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID /*lpReserved*/)
{
#ifndef REFLECTIVE_INJECT
	if (DetourIsHelperProcess())
	{
		return TRUE;
	}
	if (DLL_PROCESS_ATTACH == ul_reason_for_call)
	{
		DetourRestoreAfterWith();
		DisableThreadLibraryCalls(hModule);
		void* payload = DetourFindPayloadEx(DETOUR_INJECT_PARAMS_GUID, nullptr);
		if (!payload)
		{
			TerminateProcess(GetCurrentProcess(), 0);
		}
		const DetourInjectParams& injectParams = *static_cast<DetourInjectParams*>(payload);
		biz_initialize(injectParams.envFlag, injectParams.envPath, injectParams.envPathCount);
	}
#endif
	return TRUE;
}
