#include "sys_defs.h"
#include "detours.h"
#include "biz_initializer.h"

void biz_initialize()
{
	DetourTransactionBegin();
	LONG error = DetourTransactionCommit();
}
