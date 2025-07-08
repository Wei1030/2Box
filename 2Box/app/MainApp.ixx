export module MainApp;

import std;
import "sys_defs.h";
import Scheduler;

export int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);

export class MainApp
{
public:
	static MainApp& getInstance()
	{
		static MainApp sApp;
		return sApp;
	}

	static constexpr std::wstring_view appName{L"2Box"};
	static constexpr std::string_view appNameA{"2Box"};

	static HINSTANCE getModuleInstance()
	{
		return getInstance().m_hInstance;
	}

	static const std::wstring& getCmdLine()
	{
		return getInstance().m_strCmdLine;
	}

	static int getCmdShow()
	{
		return getInstance().m_nCmdShow;
	}

	static ID2D1Factory* getD2D1Factory()
	{
		return getInstance().m_pDirect2dFactory;
	}
	
	static IDWriteFactory* getDWriteFactory()
	{
		return getInstance().m_pDWriteFactory;
	}

	static std::wstring getFullName()
	{
		return getInstance().m_exeFullName;
	}

	static std::wstring getDir()
	{
		return getInstance().m_exeDir;
	}

	static auto get_scheduler()
	{
		return sched::Scheduler{&getInstance().m_eventLoop};
	}

private:
	MainApp();
	~MainApp();

	friend int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow);
	void initialize(HINSTANCE hInstance, std::wstring_view lpCmdLine, int nCmdShow);
	void runMessageLoop();
private:
	HINSTANCE m_hInstance{nullptr};
	std::wstring m_strCmdLine;
	int m_nCmdShow{SW_HIDE};
	std::wstring m_exeFullName;
	std::wstring m_exeDir;
	ID2D1Factory* m_pDirect2dFactory{nullptr};
	IDWriteFactory* m_pDWriteFactory{nullptr};
	sched::EventLoopForWinUi m_eventLoop;
};
