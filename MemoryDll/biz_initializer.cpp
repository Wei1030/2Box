#include "sys_defs.h"
#include "detours.h"
#include "biz_initializer.h"

import std;
import GlobalData;
import RpcClient;

void biz_initialize(unsigned long long envFlag)
{
	global::Data::get().initialize(envFlag);

	std::cout << global::Data::get().envFlag() << "\n";
	
	DetourTransactionBegin();
	LONG error = DetourTransactionCommit();

	rpc::ClientDefault c;
	c.injectToProcess(1, global::Data::get().envFlag());
}
