#pragma once

namespace rpc
{
	class Client
	{
	public:
		Client();
		~Client();

		void test();

		Client(const Client&) = delete;
		Client& operator=(const Client&) = delete;
		Client(const Client&&) = delete;
		Client& operator=(const Client&&) = delete;
	};
}
