// ReSharper disable CppFunctionalStyleCast
#include "service_h.h"
#include "RpcClient.h"

import std;

namespace rpc
{
	unsigned char* pszStringBinding = nullptr;

	Client::Client()
	{
		RPC_STATUS status = RpcStringBindingComposeA(nullptr, RPC_CSTR("ncalrpc"), nullptr, RPC_CSTR("{63B40BDA-A2D1-4516-BDBB-E1E2A960D31E}2BoxServer"), nullptr, &pszStringBinding);
		if (status != RPC_S_OK)
		{
			throw std::runtime_error(std::format("RpcStringBindingComposeA failed, return status: {}", status));
		}
		status = RpcBindingFromStringBindingA(pszStringBinding, &service_v1_0_c_ifspec);
		if (status != RPC_S_OK)
		{
			throw std::runtime_error(std::format("RpcBindingFromStringBindingA failed, return status: {}", status));
		}
	}

	Client::~Client()
	{
		RpcStringFreeA(&pszStringBinding);
		RpcBindingFree(&service_v1_0_c_ifspec);
	}

	void Client::test()
	{
		__try
		{
			std::cout << "start call" << "\n";
			inject_to_process(service_v1_0_c_ifspec, 1);
			std::cout << "end call" << "\n";
		}
		__except (RpcExceptionFilter(RpcExceptionCode()))
		{
			std::cout << "RpcExceptionCode: " << std::hex << RpcExceptionCode() << "\n";
		}
		std::cout << "end test" << "\n";
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
