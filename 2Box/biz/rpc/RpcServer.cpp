// ReSharper disable CppFunctionalStyleCast
#include "service_h.h"
#include "RpcServer.h"

import std;
import EssentialData;
import Biz.Core;
import MainApp;

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
unsigned long long login(handle_t /*IDL_handle*/, unsigned int pid, unsigned long long envFlag)
{
	try
	{
		std::shared_ptr<biz::Env> pEnv = biz::env_mgr().findEnvByFlag(envFlag);
		std::shared_ptr<biz::ProcessInfo> proc = pEnv->addProcess(pid);
		if (!proc)
		{
			throw std::runtime_error{"add fail, already added before?"};
		}
		HANDLE boxHandleInRemote{nullptr};
		if (!DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
		                     proc->getHandle(), &boxHandleInRemote,
		                     SYNCHRONIZE, FALSE, 0))
		{
			throw std::runtime_error{std::format("DuplicateHandle failed, return status:{}", GetLastError())};
		}
		return reinterpret_cast<unsigned long long>(boxHandleInRemote);
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

void request_window_inspection(handle_t /*IDL_handle*/, unsigned int pid, unsigned long long envFlag)
{
	try
	{
		std::shared_ptr<biz::Env> pEnv = biz::env_mgr().findEnvByFlag(envFlag);
		biz::wnd_enumerator().requestGetProcessAllWnd(pid, pEnv);
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

void require_elevation(handle_t /*IDL_handle*/, unsigned int pid, unsigned long long envFlag, const wchar_t path[])
{
	try
	{
		std::shared_ptr<biz::Env> pEnv = biz::env_mgr().findEnvByFlag(envFlag);
		std::shared_ptr<biz::ProcessInfo> proc = pEnv->getProcess(pid);
		if (!proc)
		{
			throw std::runtime_error{"pid not found"};
		}
		show_require_elevation_message(proc->getProcessFullPath(), path);
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

void get_all_process_id_in_env(handle_t /*IDL_handle*/, unsigned long long envFlag, unsigned long long pids[], unsigned int* count)
{
	try
	{
		std::shared_ptr<biz::Env> pEnv = biz::env_mgr().findEnvByFlag(envFlag);
		std::vector<DWORD> allPids = pEnv->getAllProcessIds();
		const unsigned int allCount = allPids.size() > MAX_PIDS ? MAX_PIDS : static_cast<unsigned int>(allPids.size());
		for (unsigned int i = 0; i < allCount; ++i)
		{
			pids[i] = allPids[i];
		}
		*count = allCount;
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

void add_toplevel_window(handle_t /*IDL_handle*/, unsigned long long hWnd, unsigned int pid, unsigned long long envFlag)
{
	try
	{
		std::shared_ptr<biz::Env> pEnv = biz::env_mgr().findEnvByFlag(envFlag);
		pEnv->addToplevelWindow(pid, reinterpret_cast<void*>(hWnd));
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

void remove_toplevel_window(handle_t /*IDL_handle*/, unsigned long long hWnd, unsigned int pid, unsigned long long envFlag)
{
	try
	{
		std::shared_ptr<biz::Env> pEnv = biz::env_mgr().findEnvByFlag(envFlag);
		pEnv->removeToplevelWindow(pid, reinterpret_cast<void*>(hWnd));
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

int contains_toplevel_window(handle_t /*IDL_handle*/, unsigned long long hWnd, unsigned long long envFlag)
{
	try
	{
		static unsigned long long hDesktopWindow = reinterpret_cast<unsigned long long>(GetDesktopWindow());
		if (hDesktopWindow == hWnd)
		{
			return 1;
		}
		std::shared_ptr<biz::Env> pEnv = biz::env_mgr().findEnvByFlag(envFlag);
		return pEnv->containsToplevelWindow(reinterpret_cast<void*>(hWnd)) ? 1 : 0;
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

int contains_toplevel_window_excluding_by_flag(handle_t /*IDL_handle*/, unsigned long long hWnd, unsigned long long excludeEnvFlag)
{
	try
	{
		return biz::env_mgr().containsToplevelWindowExcludingByFlag(reinterpret_cast<void*>(hWnd), excludeEnvFlag) ? 1 : 0;
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

void get_all_toplevel_window_exclude(handle_t /*IDL_handle*/, unsigned long long excludeEnvFlag, unsigned long long hWnds[], unsigned int* count)
{
	try
	{
		std::vector<void*> allHandles = biz::env_mgr().getAllToplevelWindowsExclude(excludeEnvFlag);
		const unsigned int allCount = allHandles.size() > MAX_TOPLEVEL_WINDOW ? MAX_TOPLEVEL_WINDOW : static_cast<unsigned int>(allHandles.size());
		for (unsigned int i = 0; i < allCount; ++i)
		{
			hWnds[i] = reinterpret_cast<unsigned long long>(allHandles[i]);
		}
		*count = allCount;
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
}

void create_redirect_file(handle_t /*IDL_handle*/, unsigned long long envFlag, const wchar_t originalFile[], const wchar_t redirectFile[])
{
	try
	{
		std::shared_ptr<biz::Env> pEnv = biz::env_mgr().findEnvByFlag(envFlag);
		
	}
	catch (...)
	{
		RpcRaiseException(0xE06D7363);
	}
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
