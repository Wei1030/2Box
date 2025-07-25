module MainApp;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import UI.MainWindow;

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

		ui::MainWindow mainWnd;
		ui::g_main_wnd = &mainWnd;

		mainWnd.show(app.cmdShow());

		app.runMessageLoop();
	}
	catch (const std::exception& e)
	{
		// 由于app的析构会触发PostQuitMessage
		// 所以需要在 MessageBox 前清除WM_QUIT消息，否则MessageBox窗口会立即销毁并返回
		clear_quit_msg();
		MessageBoxA(nullptr, e.what(), MainApp::appNameA.data(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	catch (...)
	{
		clear_quit_msg();
		MessageBoxW(nullptr, L"Unknown error", MainApp::appName.data(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
	}
	return 0;
}
