#include "sys_defs.h"
#include "biz_initializer.h"

import std;
import GlobalData;
import RpcClient;
import Hook;

void initialize_global_data(unsigned long long envFlag, unsigned long envIndex, std::wstring_view rootPath)
{
	global::Data::get().initialize(envFlag, envIndex, rootPath);
}

void initialize_rpc()
{
	const rpc::ClientDefault c;
	c.addToBox(GetCurrentProcessId(), global::Data::get().envFlag());
}

void initialize_hook()
{
	hook::hook_all();
}

void biz_initialize(unsigned long long envFlag, unsigned long envIndex, const wchar_t* rootPath, DWORD rootPathCount)
{
	try
	{
		initialize_global_data(envFlag, envIndex, std::wstring_view{rootPath, rootPathCount});
		initialize_rpc();
		initialize_hook();
	}
	catch (...)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
}
