// ReSharper disable CppFunctionalStyleCast
#include "service_h.h"
#include "RpcServer.h"

import std;
import Injector;
import EssentialData;

namespace rpc
{
	Server::Server()
	{
		RPC_STATUS status = RpcServerUseProtseqEpA(RPC_CSTR("ncalrpc"), RPC_C_LISTEN_MAX_CALLS_DEFAULT, RPC_CSTR("{63B40BDA-A2D1-4516-BDBB-E1E2A960D31E}2BoxServer"), nullptr);
		if (status != RPC_S_OK)
		{
			if (status == RPC_S_DUPLICATE_ENDPOINT)
			{
				MessageBoxW(nullptr, L"2Box已经运行", L"2Box", MB_OK);
				throw std::runtime_error{""};
			}
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
void inject_to_process(handle_t IDL_handle, unsigned int pid, unsigned long long envFlag)
{
	try
	{
		biz::inject_memory_dll_to_process(pid, envFlag, biz::get_injection_dlls(), biz::get_essential_data());
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

void get_all_process_id_in_env(handle_t IDL_handle, unsigned long long envFlag, unsigned long long pids[], unsigned int* count)
{
	*count = 0;
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
