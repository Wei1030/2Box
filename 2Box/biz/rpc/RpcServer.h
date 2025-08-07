#pragma once

namespace rpc
{
	class Server
	{
	public:
		Server();
		~Server();

		Server(const Server&) = delete;
		Server& operator=(const Server&) = delete;
		Server(Server&&) = delete;
		Server& operator=(Server&&) = delete;
	};

	
}
