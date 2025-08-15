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

void initialize_global_data(unsigned long long envFlag, std::wstring_view envDir)
{
	global::Data::get().initialize(envFlag, envDir);
}

void initialize_hook()
{
	hook::hook_all();
}

void biz_initialize(unsigned long long envFlag, const wchar_t* envDir, DWORD envDirCount)
{
	try
	{
		initialize_rpc();
		initialize_global_data(envFlag, std::wstring_view{envDir, envDirCount});
		initialize_hook();
	}
	catch (...)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
}
