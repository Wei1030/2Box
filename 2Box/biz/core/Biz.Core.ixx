module;
#include "biz/rpc/RpcServer.h"
export module Biz.Core;

import std;
export import Env;
export import Launcher;

namespace biz
{
	inline EnvManager* g_env_mgr{nullptr};
	inline Launcher* g_launcher{nullptr};
	inline std::unique_ptr<rpc::Server> g_rpc_server{nullptr};

	export class Core
	{
	public:
		Core()
		{
			g_env_mgr = &m_envManager;
			g_launcher = &m_launcher;
			g_rpc_server = std::make_unique<rpc::Server>();
		}

		~Core()
		{
			g_rpc_server.reset();
		}

	private:
		EnvManager m_envManager;
		Launcher m_launcher;
	};

	export EnvManager& env_mgr()
	{
		return *g_env_mgr;
	}

	export Launcher& launcher()
	{
		return *g_launcher;
	}

	export void shutdown_rpc_server()
	{
		g_rpc_server.reset();
	}
}
