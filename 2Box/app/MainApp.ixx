export module MainApp;

import std;
import "sys_defs.h";
import Scheduler;

export int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);
export class MainApp;

export struct AppCommonTextFormat
{
	IDWriteTextFormat* pTitleFormat{nullptr};
	IDWriteTextFormat* pMainFormat{nullptr};
	IDWriteTextFormat* pErrorMsgFormat{nullptr};
	IDWriteTextFormat* pTipsFormat{nullptr};
	IDWriteTextFormat* pToolBtnFormat{nullptr};

	IDWriteInlineObject* pTitleEllipsisTrimmingSign{nullptr};
	IDWriteInlineObject* pMainEllipsisTrimmingSign{nullptr};
	IDWriteInlineObject* pErrorMsgEllipsisTrimmingSign{nullptr};
	IDWriteInlineObject* pTipsEllipsisTrimmingSign{nullptr};
	IDWriteInlineObject* pToolBtnEllipsisTrimmingSign{nullptr};

	~AppCommonTextFormat()
	{
		safe_release(&pTitleFormat);
		safe_release(&pMainFormat);
		safe_release(&pErrorMsgFormat);
		safe_release(&pTipsFormat);
		safe_release(&pToolBtnFormat);

		safe_release(&pTitleEllipsisTrimmingSign);
		safe_release(&pMainEllipsisTrimmingSign);
		safe_release(&pErrorMsgEllipsisTrimmingSign);
		safe_release(&pTipsEllipsisTrimmingSign);
		safe_release(&pToolBtnEllipsisTrimmingSign);
	}

	void setAllTextEllipsisTrimming() const
	{
		setTitleEllipsisTrimming();
		setMainEllipsisTrimming();
		setErrorMsgEllipsisTrimming();
		setTipsEllipsisTrimming();
		setToolBtnEllipsisTrimming();
	}

	void clearAllTextEllipsisTrimming() const
	{
		clearTitleTrimming();
		clearMainEllipsisTrimming();
		clearErrorMsgTrimming();
		clearTipsEllipsisTrimming();
		clearToolBtnEllipsisTrimming();
	}

	void setTitleEllipsisTrimming() const
	{
		pTitleFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER};
		pTitleFormat->SetTrimming(&trimming, pTitleEllipsisTrimmingSign);
	}

	void clearTitleTrimming() const
	{
		pTitleFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_NONE};
		pTitleFormat->SetTrimming(&trimming, nullptr);
	}

	void setMainEllipsisTrimming() const
	{
		pMainFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER};
		pMainFormat->SetTrimming(&trimming, pMainEllipsisTrimmingSign);
	}

	void clearMainEllipsisTrimming() const
	{
		pMainFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_NONE};
		pMainFormat->SetTrimming(&trimming, nullptr);
	}

	void setErrorMsgEllipsisTrimming() const
	{
		pErrorMsgFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER};
		pErrorMsgFormat->SetTrimming(&trimming, pErrorMsgEllipsisTrimmingSign);
	}

	void clearErrorMsgTrimming() const
	{
		pErrorMsgFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_NONE};
		pErrorMsgFormat->SetTrimming(&trimming, nullptr);
	}

	void setTipsEllipsisTrimming() const
	{
		pTipsFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER};
		pTipsFormat->SetTrimming(&trimming, pTipsEllipsisTrimmingSign);
	}

	void clearTipsEllipsisTrimming() const
	{
		pTipsFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_NONE};
		pTipsFormat->SetTrimming(&trimming, nullptr);
	}

	void setToolBtnEllipsisTrimming() const
	{
		pToolBtnFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_CHARACTER};
		pToolBtnFormat->SetTrimming(&trimming, pToolBtnEllipsisTrimmingSign);
	}

	void clearToolBtnEllipsisTrimming() const
	{
		pToolBtnFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
		constexpr DWRITE_TRIMMING trimming{DWRITE_TRIMMING_GRANULARITY_NONE};
		pToolBtnFormat->SetTrimming(&trimming, nullptr);
	}

private:
	friend class MainApp;

	static void createCommonTextAbout(IDWriteFactory* pDWriteFactory, DWRITE_FONT_WEIGHT weight, float fontSize, IDWriteTextFormat** ppTextFormat, IDWriteInlineObject** ppEllipsisTrimmingSign)
	{
		HRESULT hr = pDWriteFactory->CreateTextFormat(
			L"Segoe UI",
			nullptr,
			weight,
			DWRITE_FONT_STYLE_NORMAL,
			DWRITE_FONT_STRETCH_NORMAL,
			fontSize,
			L"",
			ppTextFormat);
		if (FAILED(hr))
		{
			throw std::runtime_error(std::format("CreateTextFormat fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
		}

		hr = pDWriteFactory->CreateEllipsisTrimmingSign(
			*ppTextFormat,
			ppEllipsisTrimmingSign
		);
		if (FAILED(hr))
		{
			throw std::runtime_error(std::format("CreateEllipsisTrimmingSign fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
		}
	}

	void createAllCommonTextFormat(IDWriteFactory* pDWriteFactory)
	{
		createCommonTextAbout(pDWriteFactory, DWRITE_FONT_WEIGHT_SEMI_BOLD, 16.f, &pTitleFormat, &pTitleEllipsisTrimmingSign);
		createCommonTextAbout(pDWriteFactory, DWRITE_FONT_WEIGHT_NORMAL, 14.f, &pMainFormat, &pMainEllipsisTrimmingSign);
		createCommonTextAbout(pDWriteFactory, DWRITE_FONT_WEIGHT_NORMAL, 13.f, &pErrorMsgFormat, &pErrorMsgEllipsisTrimmingSign);
		createCommonTextAbout(pDWriteFactory, DWRITE_FONT_WEIGHT_NORMAL, 12.f, &pTipsFormat, &pTipsEllipsisTrimmingSign);
		createCommonTextAbout(pDWriteFactory, DWRITE_FONT_WEIGHT_NORMAL, 11.f, &pToolBtnFormat, &pToolBtnEllipsisTrimmingSign);
	}
};

class MainApp
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

	const AppCommonTextFormat& textFormat() const noexcept
	{
		return m_commonTextFormat;
	}

	std::wstring_view exeFullName() const noexcept
	{
		return m_exeFullName;
	}

	std::wstring_view exeDir() const noexcept
	{
		return m_exeDir;
	}

	void exit() const noexcept
	{
		m_eventLoop.finish();
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

		m_commonTextFormat.createAllCommonTextFormat(m_pDWriteFactory);

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

	void runMessageLoop() const
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
	AppCommonTextFormat m_commonTextFormat;
	mutable sched::EventLoopForWinUi m_eventLoop;
};

inline MainApp* g_app{nullptr};

export MainApp& app() noexcept
{
	return *g_app;
}
