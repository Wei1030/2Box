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
		MainApp::getInstance().initialize(hInstance, lpCmdLine, nCmdShow);

		ui::MainWindow mainWnd;
		ui::g_main_wnd = &mainWnd;
		mainWnd.show(MainApp::getCmdShow());

		MainApp::getInstance().runMessageLoop();
	}
	catch (const std::exception& e)
	{
		MessageBoxA(nullptr, e.what(), MainApp::appNameA.data(), MB_OK);
	}
	return 0;
}

MainApp::MainApp()
{
	HRESULT hr = CoInitialize(nullptr);
	if (FAILED(hr))
	{
		throw std::runtime_error(std::format("CoInitialize fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
	}
	
	hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
	if (FAILED(hr))
	{
		throw std::runtime_error(std::format("D2D1CreateFactory fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
	}
	
	hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), 	reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
	if (FAILED(hr))
	{
		throw std::runtime_error(std::format("DWriteCreateFactory fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
	}
}

MainApp::~MainApp()
{
	safe_release(&m_pDirect2dFactory);
	CoUninitialize();
}

void MainApp::initialize(HINSTANCE hInstance, std::wstring_view lpCmdLine, int nCmdShow)
{
	m_hInstance = hInstance;
	m_strCmdLine = lpCmdLine;
	m_nCmdShow = nCmdShow;

	wchar_t szFullName[MAX_PATH + 1] = { 0 };
	GetModuleFileNameW(nullptr, szFullName, MAX_PATH);
	m_exeFullName = szFullName;

	namespace fs = std::filesystem;
	const fs::path fsPath = fs::absolute(fs::path(m_exeFullName));
	m_exeDir = fsPath.parent_path().native();
}

void MainApp::runMessageLoop()
{
	m_eventLoop.run();
}
