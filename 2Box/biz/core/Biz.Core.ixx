module;
#include "biz/rpc/RpcServer.h"
export module Biz.Core;

import std;
export import Env;
export import Launcher;
export import WndEnumerator;
export import FileRedirect;

namespace biz
{
	inline FileRedirect* g_file_redirect{nullptr};
	inline EnvManager* g_env_mgr{nullptr};
	inline Launcher* g_launcher{nullptr};
	inline WndEnumerator* g_wnd_enumerator{nullptr};
	inline std::unique_ptr<rpc::Server> g_rpc_server{nullptr};

	export class Core
	{
	public:
		Core()
		{
			g_file_redirect = &m_fileRedirect;
			g_env_mgr = &m_envManager;
			g_launcher = &m_launcher;
			g_wnd_enumerator = &m_wndEnumerator;
			g_rpc_server = std::make_unique<rpc::Server>();
		}

		~Core()
		{
			g_rpc_server.reset();
		}

	private:
		FileRedirect m_fileRedirect;
		EnvManager m_envManager;
		Launcher m_launcher;
		WndEnumerator m_wndEnumerator;
	};

	export FileRedirect& file_redirect()
	{
		return *g_file_redirect;
	}

	export EnvManager& env_mgr()
	{
		return *g_env_mgr;
	}

	export Launcher& launcher()
	{
		return *g_launcher;
	}

	export WndEnumerator& wnd_enumerator()
	{
		return *g_wnd_enumerator;
	}

	export void shutdown_rpc_server()
	{
		g_rpc_server.reset();
	}
}
