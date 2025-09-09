#include "sys_defs.h"
#include "biz_initializer.h"

import std;
import GlobalData;
import RpcClient;
import Hook;

int filter_login_error(unsigned int code)
{
	if (RpcExceptionFilter(code) == EXCEPTION_CONTINUE_SEARCH)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	if (code == 0xE06D7363)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	return EXCEPTION_EXECUTE_HANDLER;
}

HANDLE login_two_box()
{
	__try
	{
		const rpc::ClientDefault c;
		unsigned long long boxHandle = c.addToBox(GetCurrentProcessId(), global::Data::get().envFlag());
		return reinterpret_cast<HANDLE>(boxHandle);
	}
	__except (filter_login_error(RpcExceptionCode()))
	{
		return nullptr;
	}
}

void initialize_global_data(unsigned long long envFlag, unsigned long envIndex, std::wstring_view rootPath)
{
	global::Data::get().initialize(envFlag, envIndex, rootPath);
}

void initialize_rpc()
{
	struct BoxSimpleWatcher
	{
		HANDLE quitEvent;
		std::thread thread;

		explicit BoxSimpleWatcher(HANDLE boxHandle)
		{
			quitEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);
			if (!quitEvent)
			{
				throw std::runtime_error(std::format("CreateEvent failed: {}", GetLastError()));
			}
			thread = std::thread([this, boxHandle]
			{
				watchLoop(boxHandle);
			});
		}

		~BoxSimpleWatcher()
		{
			if (thread.joinable())
			{
				if (SetEvent(quitEvent))
				{
					thread.join();
				}
				else
				{
					thread.detach();
				}
			}
			CloseHandle(quitEvent);
		}

		void watchLoop(HANDLE boxHandle) const
		{
			while (true)
			{
				if (!boxHandle)
				{
					try
					{
						boxHandle = login_two_box();
					}
					catch (...)
					{
						break;
					}
				}
				if (boxHandle)
				{
					std::vector handles{quitEvent, boxHandle};
					DWORD index = WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, INFINITE);
					if (index >= handles.size())
					{
						break;
					}
					if (handles[index] == quitEvent)
					{
						break;
					}
					CloseHandle(boxHandle);
					boxHandle = nullptr;
				}
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
	};

	try
	{
		static BoxSimpleWatcher watcher{login_two_box()};
	}
	catch (...)
	{
	}
}

void initialize_hook()
{
	hook::hook_all();
}

void biz_initialize(unsigned long long envFlag, unsigned long envIndex, const wchar_t* rootPath, DWORD rootPathCount)
{
	try
	{
		initialize_global_data(envFlag, envIndex, std::wstring_view{rootPath, rootPathCount});
		initialize_rpc();
		initialize_hook();
	}
	catch (...)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
}
