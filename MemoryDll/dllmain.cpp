#ifndef _WIN64
#pragma comment(linker, "/EXPORT:initialize=_initialize@4")
#endif

import "sys_defs.h";
import std;
import PELoader;

#include "biz_initializer.h"

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

	biz_initialize(injectParams.envFlag);
	return 0;
}

BOOL APIENTRY DllMain(HMODULE /*hModule*/, DWORD /*ul_reason_for_call*/, LPVOID /*lpReserved*/)
{
	return TRUE;
}
