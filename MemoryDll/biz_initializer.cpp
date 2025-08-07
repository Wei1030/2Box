#include "sys_defs.h"
#include "detours.h"
#include "biz_initializer.h"
#include "rpc/RpcClient.h"

void biz_initialize()
{
	DetourTransactionBegin();
	LONG error = DetourTransactionCommit();

	rpc::Client c;
	c.test();
}
