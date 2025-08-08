#include "service_h.h"
// midl要求实现的函数:
extern "C" {
void* __RPC_USER midl_user_allocate(size_t size)
{
	return malloc(size);
}

void __RPC_USER midl_user_free(void* ptr)
{
	free(ptr);
}
}
