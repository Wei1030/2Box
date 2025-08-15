module;
#include "biz/rpc/RpcServer.h"
module MainApp;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import UI.MainWindow;
import EssentialData;
import Env;

namespace
{
	void clear_quit_msg() noexcept
	{
		MSG msg;
		PeekMessageW(&msg, nullptr, WM_QUIT, WM_QUIT, PM_REMOVE);
	}
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ [[maybe_unused]] HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
	try
	{
		MainApp app{hInstance, lpCmdLine, nCmdShow};
		g_app = &app;
		
		biz::Reg::instance().initialize();
#ifndef _WIN64
#if 0
		if (!biz::get_essential_data().version.is32BitSystem)
		{
			MessageBoxW(nullptr, L"请使用本软件的64位版本", MainApp::appName.data(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return 0;
		}
#endif
#endif

		ui::MainWindow mainWnd;
		ui::g_main_wnd = &mainWnd;

		rpc::Server server;

		mainWnd.show(app.cmdShow());

		app.runMessageLoop();
	}
	catch (const std::exception& e)
	{
		// 由于app的析构会触发PostQuitMessage
		// 所以需要在 MessageBox 前清除WM_QUIT消息，否则MessageBox窗口会立即销毁并返回
		clear_quit_msg();
		std::string_view msg = e.what();
		if (!msg.empty())
		{
			MessageBoxA(nullptr, msg.data(), MainApp::appNameA.data(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
		}
	}
	catch (...)
	{
		clear_quit_msg();
		MessageBoxW(nullptr, L"Unknown error", MainApp::appName.data(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	return 0;
}
