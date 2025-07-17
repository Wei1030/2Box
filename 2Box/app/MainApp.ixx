export module MainApp;

import std;
import "sys_defs.h";
import Scheduler;

export int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);

export class MainApp
{
public:
	static constexpr std::wstring_view appName{L"2Box"};
	static constexpr std::string_view appNameA{"2Box"};

public:
	HINSTANCE moduleInstance() const noexcept
	{
		return m_hInstance;
	}

	std::wstring_view cmdLine() const noexcept
	{
		return m_strCmdLine;
	}

	int cmdShow() const noexcept
	{
		return m_nCmdShow;
	}

	ID2D1Factory* d2d1Factory() const noexcept
	{
		return m_pDirect2dFactory;
	}

	IDWriteFactory* dWriteFactory() const noexcept
	{
		return m_pDWriteFactory;
	}

	std::wstring_view exeFullName() const noexcept
	{
		return m_exeFullName;
	}

	std::wstring_view exeDir() const noexcept
	{
		return m_exeDir;
	}

	// ReSharper disable CppInconsistentNaming
	auto get_scheduler() const noexcept
	{
		return sched::Scheduler<sched::EventLoopForWinUi>{&m_eventLoop};
	}

	// ReSharper restore CppInconsistentNaming

private:
	friend int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);

	MainApp(HINSTANCE hInstance, std::wstring_view lpCmdLine, int nCmdShow)
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

		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&m_pDWriteFactory));
		if (FAILED(hr))
		{
			throw std::runtime_error(std::format("DWriteCreateFactory fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
		}

		m_hInstance = hInstance;
		m_strCmdLine = lpCmdLine;
		m_nCmdShow = nCmdShow;

		wchar_t szFullName[MAX_PATH + 1] = {0};
		GetModuleFileNameW(nullptr, szFullName, MAX_PATH);
		m_exeFullName = szFullName;

		namespace fs = std::filesystem;
		const fs::path fsPath = fs::absolute(fs::path(m_exeFullName));
		m_exeDir = fsPath.parent_path().native();
	}

	~MainApp()
	{
		safe_release(&m_pDWriteFactory);
		safe_release(&m_pDirect2dFactory);
		CoUninitialize();
	}

	void runMessageLoop()
	{
		m_eventLoop.run();
	}

private:
	HINSTANCE m_hInstance{nullptr};
	std::wstring m_strCmdLine;
	int m_nCmdShow{SW_HIDE};
	std::wstring m_exeFullName;
	std::wstring m_exeDir;
	ID2D1Factory* m_pDirect2dFactory{nullptr};
	IDWriteFactory* m_pDWriteFactory{nullptr};
	mutable sched::EventLoopForWinUi m_eventLoop;
};

MainApp* g_app{nullptr};

export MainApp& app() noexcept
{
	return *g_app;
}
