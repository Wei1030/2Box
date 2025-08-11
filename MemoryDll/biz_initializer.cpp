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

void initialize_global_data(unsigned long long envFlag)
{
	global::Data::get().initialize(envFlag);
}

void initialize_hook()
{
	hook::hook_all();
}

void biz_initialize(unsigned long long envFlag)
{
	try
	{
		initialize_rpc();
		initialize_global_data(envFlag);
		initialize_hook();		
	}
	catch (...)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
}
