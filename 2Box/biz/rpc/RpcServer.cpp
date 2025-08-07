// ReSharper disable CppFunctionalStyleCast
#include "service_h.h"
#include "RpcServer.h"
import std;

namespace rpc
{
	Server::Server()
	{
		RPC_STATUS status = RpcServerUseProtseqEpA(RPC_CSTR("ncalrpc"), RPC_C_LISTEN_MAX_CALLS_DEFAULT, RPC_CSTR("Rpc2BoxServer"), nullptr);
		if (status != RPC_S_OK)
		{
			// ReSharper disable once StringLiteralTypo
			throw std::runtime_error(std::format("RpcServerUseProtseqEp failed, return status:{}", status));
		}
	
		status = RpcServerRegisterIf(service_v1_0_s_ifspec, nullptr, nullptr);
		if (status != RPC_S_OK)
		{
			throw std::runtime_error(std::format("RpcServerRegisterIf failed, return status:{}", status));
		}
	
		status = RpcServerListen(1, RPC_C_LISTEN_MAX_CALLS_DEFAULT, TRUE);
		if (status != RPC_S_OK)
		{
			RpcServerUnregisterIf(nullptr, nullptr, FALSE);
			throw std::runtime_error(std::format("RpcServerListen failed, return status:{}", status));
		}
	}

	Server::~Server()
	{
		if (const RPC_STATUS status = RpcMgmtStopServerListening(nullptr); status == RPC_S_OK)
		{
			RpcMgmtWaitServerListen();
			RpcServerUnregisterIf(nullptr, nullptr, FALSE);
		}
	}	
}


extern "C" {
void inject_to_process(handle_t IDL_handle, unsigned int pid)
{
	// throw std::runtime_error("inject_to_process: not implemented");
	RpcRaiseException(ERROR_CALL_NOT_IMPLEMENTED);
}
}

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
