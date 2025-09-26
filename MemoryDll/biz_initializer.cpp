#include "biz_initializer.h"

import std;
import GlobalData;
import RpcClient;
import Hook;

int filter_offline_error(unsigned int code)
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
		unsigned long long boxHandle = c.login2Box(GetCurrentProcessId(), global::Data::get().envFlag());
		return reinterpret_cast<HANDLE>(boxHandle);
	}
	__except (filter_offline_error(RpcExceptionCode()))
	{
		return nullptr;
	}
}

void request_window_inspection()
{
	__try
	{
		const rpc::ClientDefault c;
		c.requestWindowInspection(GetCurrentProcessId(), global::Data::get().envFlag());
	}
	__except (filter_offline_error(RpcExceptionCode()))
	{
	}
}

void initialize_global_data(SystemVersionInfo versionInfo, unsigned long long envFlag, unsigned long envIndex, std::wstring_view rootPath)
{
	global::Data::get().initialize(versionInfo, envFlag, envIndex, rootPath);
}

// 目前虽然处理了2box临时退出的情况, 但是当进程重新登录2box后必定会存在一段时间无法阻止其访问其他env内的窗口（因为其他env内的进程也可能正在登录），这可能会引起问题
// 从另一方面讲，允许2box临时退出其实没什么必要，唯一的好处是当有进程需要以管理员身份启动子进程时可以通知2box以管理员身份重启并作为父进程启动需要管理员权限的进程。。。没什么必要，目前直接全部强制退出就行，以后再说吧，
// 所以暂时不允许2Box退出。
#define ALLOW_2BOX_EXIT 0

void initialize_rpc()
{
	struct BoxSimpleWatcher
	{
		HANDLE quitEvent;
		std::thread thread;

		explicit BoxSimpleWatcher(HANDLE boxHandle)
		{
#if !ALLOW_2BOX_EXIT
			if (boxHandle == nullptr)
			{
				TerminateProcess(GetCurrentProcess(), 0);
			}
#endif
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
						if (boxHandle)
						{
							request_window_inspection();
						}
					}
					catch (...)
					{
						TerminateProcess(GetCurrentProcess(), 0);
					}
				}
				if (boxHandle)
				{
					std::vector handles{quitEvent, boxHandle};
					DWORD index = WaitForMultipleObjects(static_cast<DWORD>(handles.size()), handles.data(), FALSE, INFINITE);
					if (index >= handles.size())
					{
						TerminateProcess(GetCurrentProcess(), 0);
					}
					if (handles[index] == quitEvent)
					{
						break;
					}
					CloseHandle(boxHandle);
					boxHandle = nullptr;
				}
#if !ALLOW_2BOX_EXIT
				TerminateProcess(GetCurrentProcess(), 0);
#endif
				std::this_thread::sleep_for(std::chrono::seconds(1));
			}
		}
	};

	static BoxSimpleWatcher watcher{login_two_box()};
}

void initialize_hook()
{
	hook::hook_all();
}

void biz_initialize(SystemVersionInfo versionInfo, unsigned long long envFlag, unsigned long envIndex, const wchar_t* rootPath, DWORD rootPathCount)
{
	try
	{
		initialize_global_data(versionInfo, envFlag, envIndex, std::wstring_view{rootPath, rootPathCount});
		initialize_rpc();
		initialize_hook();
	}
	catch (...)
	{
		TerminateProcess(GetCurrentProcess(), 0);
	}
}
