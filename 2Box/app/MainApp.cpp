module MainApp;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import UI.MainWindow;

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
		MessageBoxA(nullptr, e.what(), MainApp::appNameA.data(), MB_OK);
	}
	return 0;
}
