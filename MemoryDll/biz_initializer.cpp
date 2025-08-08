#include "sys_defs.h"
#include "detours.h"
#include "biz_initializer.h"

import RpcClient;

void biz_initialize()
{
	DetourTransactionBegin();
	LONG error = DetourTransactionCommit();

	rpc::ClientDefault c;
	c.injectToProcess(1);
}
