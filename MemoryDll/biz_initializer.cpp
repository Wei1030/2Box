#include "sys_defs.h"
#include "biz_initializer.h"

import std;
import GlobalData;
import RpcClient;
import Hook;

void initialize_rpc()
{
	rpc::get_default_binding_info();
}

void initialize_global_data(unsigned long long envFlag, std::wstring_view envPath)
{
	global::Data::get().initialize(envFlag, envPath);
}

void initialize_hook()
{
	hook::hook_all();
}

void biz_initialize(unsigned long long envFlag, const wchar_t* envPath, DWORD envPathCount)
{
	try
	{
		initialize_rpc();
		initialize_global_data(envFlag, std::wstring_view{envPath, envPathCount});
		initialize_hook();
	}
	catch (...)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
}
