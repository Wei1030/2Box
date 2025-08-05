#ifndef _WIN64
#pragma comment(linker, "/EXPORT:initialize=_initialize@4")
#endif

import "sys_defs.h";
import std;
import PELoader;

#include "biz_initializer.h"

std::unique_ptr<pe::MemoryModule> g_this_module;
const ReflectiveInjectParams* g_injection_ctx{nullptr};

const ReflectiveInjectParams& get_injection_ctx()
{
	return *g_injection_ctx;
}

void make_exception1()
{
	std::cout << __FUNCTION__ << ": Throwing exception:" << std::endl;
	__try
	{
		RaiseException(STATUS_BREAKPOINT, 0, 0, 0);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		std::cout << "Exception handled: STATUS_BREAKPOINT" << std::endl;
	}
}

void make_exception2()
{
	std::cout << __FUNCTION__ << ": Throwing exception:" << std::endl;
	try
	{
		RaiseException(STATUS_INTEGER_DIVIDE_BY_ZERO, 0, 0, 0);
	}
	catch (...)
	{
		std::cout << "Exception handled: STATUS_INTEGER_DIVIDE_BY_ZERO" << std::endl;
	}
}

void test_throw()
{
	try
	{
		throw std::runtime_error("test throw!");
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << "\n";
	}
	catch (...)
	{
		std::cout << "catch ..." << "\n";
	}
}

int test_static_init()
{
	struct MyStruct
	{
		MyStruct()
		{
			std::cout << "static_init!!!!!\n";
			i = 42;
		}

		int i = 0;
	};
	static MyStruct s;
	return s.i;
}

extern "C" __declspec(dllexport) unsigned long __stdcall initialize(void* lpThreadParameter)
{
	if (!lpThreadParameter)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
	g_injection_ctx = static_cast<ReflectiveInjectParams*>(lpThreadParameter);

	const EssentialData& essentialData = g_injection_ctx->essentialData;
	const pe::MemoryModule& thisModule = *g_this_module;

	pe::fill_os_version(essentialData.version);
	pe::fill_all_symbols(essentialData.symRva32, essentialData.symRva64);

	pe::set_section_protection(thisModule);
	pe::enable_exceptions(thisModule);
	if (!pe::init_static_tls(thisModule))
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
	pe::flush_instruction_cache();

	std::cout << test_static_init() << "\n";
	std::thread{
		[]()
		{
			std::cout << test_static_init() << "\n";
		}
	}.detach();

	test_throw();
	make_exception1();
	make_exception2();

	biz_initialize();
	return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	std::cout << "entry DllMain,reason: " << ul_reason_for_call << "\n";
	if (ul_reason_for_call == DLL_PROCESS_ATTACH)
	{
		std::cout << "Injected!" << "\n";

		g_this_module = pe::create_module_from_mapped_memory(hModule);

		std::cout << "create!" << "\n";
	}
	return TRUE;
}
