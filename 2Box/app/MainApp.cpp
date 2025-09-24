module;
#pragma comment(lib, "Comctl32.lib")
module MainApp;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import UI.MainWindow;
import EssentialData;
import Biz.Core;

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
		biz::get_essential_data();
#ifndef _WIN64
#if 0
		if (!biz::get_essential_data().version.is32BitSystem)
		{
			MessageBoxW(nullptr, L"请使用本软件的64位版本", MainApp::appName.data(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return 0;
		}
#endif
#endif

		MainApp app{hInstance, lpCmdLine, nCmdShow};
		g_app = &app;

		biz::Core biz;

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

void MainApp::parseCmdLine() const
{
	if (m_strCmdLine.empty())
	{
		return;
	}
	int numArgs;
	LPWSTR* cmdArray = CommandLineToArgvW(m_strCmdLine.c_str(), &numArgs);
	if (!cmdArray)
	{
		return;
	}
	for (int i = 0; i < numArgs; ++i)
	{
		std::wstring_view arg{cmdArray[i]};
		std::size_t delimiterPos = arg.find(L'=');
		if (delimiterPos != std::wstring_view::npos)
		{
			std::wstring_view paramName = arg.substr(0, delimiterPos);
			if (paramName == L"--wait-another-end")
			{
				std::wstring_view paramValue = arg.substr(delimiterPos + 1);
				waitAnotherInstanceEnd(paramValue);
			}
		}
	}
	LocalFree(cmdArray);
}

void MainApp::waitAnotherInstanceEnd(std::wstring_view strPid) const
{
	HANDLE hProcess{nullptr};

	try
	{
		DWORD pid = std::stoul(std::wstring{strPid});
		hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | SYNCHRONIZE, FALSE, pid);
		if (!hProcess)
		{
			throw std::runtime_error(std::format("Failed to open process, error:{}", GetLastError()));
		}
		DWORD pathLength = std::numeric_limits<short>::max();
		std::wstring path;
		path.resize(pathLength);
		if (!QueryFullProcessImageNameW(hProcess, 0, path.data(), &pathLength))
		{
			throw std::runtime_error(std::format("Failed to query full process image, error:{}", GetLastError()));
		}
		path.resize(pathLength);
		path = std::wstring(path);
		if (_wcsicmp(m_exeFullName.c_str(), path.c_str()) == 0)
		{
			WaitForSingleObject(hProcess, INFINITE);
		}
	}
	catch (...)
	{
	}
	if (hProcess)
	{
		CloseHandle(hProcess);
	}
}

namespace
{
	HRESULT CALLBACK task_dialog_callback(_In_ HWND hwnd, _In_ UINT msg, _In_ WPARAM wParam, _In_ LPARAM lParam, _In_ LONG_PTR lpRefData)
	{
		if (msg == TDN_CREATED)
		{
			SendMessageW(hwnd, TDM_SET_BUTTON_ELEVATION_REQUIRED_STATE, IDYES, 1);
		}
		else if (msg == TDN_BUTTON_CLICKED)
		{
			if (wParam == IDYES)
			{
				SHELLEXECUTEINFOW siw{sizeof(siw)};
				siw.lpVerb = L"runas";
				siw.lpFile = app().exeFullName().data();
				std::wstring params = std::format(LR"(--wait-another-end={})", GetCurrentProcessId());
				siw.lpParameters = params.c_str();
				siw.nShow = SW_SHOWNORMAL;
				ShellExecuteExW(&siw);
				app().exit();
			}
		}
		return S_OK;
	}
}

void show_require_elevation_message(std::wstring_view requester, std::wstring_view path)
{
	static std::atomic hasShown{false};
	if (hasShown.exchange(true))
	{
		return;
	}
	app().get_scheduler().addTask([requester = std::wstring{requester}, path = std::wstring{path}]()
	{
		TASKDIALOGCONFIG cfg{sizeof(TASKDIALOGCONFIG)};
		cfg.hwndParent = ui::main_wnd().nativeHandle();
		cfg.dwFlags = TDF_USE_COMMAND_LINKS | TDF_ALLOW_DIALOG_CANCELLATION | TDF_POSITION_RELATIVE_TO_WINDOW;
		cfg.pszWindowTitle = MainApp::appName.data();
		cfg.pszMainIcon = TD_SHIELD_ICON;
		cfg.pszMainInstruction = L"环境中有进程试图以管理员身份运行子进程";
		std::wstring content = std::format(L"进程：{}\n试图以管理员身份运行：{}\n\n"
		                                   L"{}在权限受限情况下会始终阻止环境中的进程以管理员身份创建子进程，这可能会导致环境中的进程功能或行为不正常。您可以选择：",
		                                   requester, path, MainApp::appName.data());
		cfg.pszContent = content.c_str();
		cfg.dwCommonButtons = TDCBF_CANCEL_BUTTON;
		std::wstring btnText1 = std::format(L"以管理员身份重新运行{}\n"
		                                    L"注意！重启{}会导致强制关闭所有环境中的所有进程！建议手动关闭所有进程以免丢失数据",
		                                    MainApp::appName.data(), MainApp::appName.data());
		std::wstring btnText2 = std::format(L"必要时手动以管理员身份运行{}\n"
		                                    L"选择此项并继续观察环境中该进程的表现，如您发现任何异常，可以手动以管理员身份重新运行{}",
		                                    MainApp::appName.data(), MainApp::appName.data());
		TASKDIALOG_BUTTON btnArray[2]{};
		btnArray[0].nButtonID = IDYES;
		btnArray[0].pszButtonText = btnText1.c_str();
		btnArray[1].nButtonID = IDCANCEL;
		btnArray[1].pszButtonText = btnText2.c_str();
		cfg.pButtons = btnArray;
		cfg.cButtons = sizeof(btnArray) / sizeof(TASKDIALOG_BUTTON);
		cfg.nDefaultButton = IDCANCEL;
		cfg.pfCallback = task_dialog_callback;

		TaskDialogIndirect(&cfg, nullptr, nullptr, nullptr);
	});
}
