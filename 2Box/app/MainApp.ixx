export module MainApp;

import std;
import "sys_defs.h";
import Scheduler;

export int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow);
export class MainApp;

export struct AppCommonTextFormat
{
	UniqueComPtr<IDWriteTextFormat> pTitleFormat;
	UniqueComPtr<IDWriteTextFormat> pMainFormat;
	UniqueComPtr<IDWriteTextFormat> pErrorMsgFormat;
	UniqueComPtr<IDWriteTextFormat> pTipsFormat;
	UniqueComPtr<IDWriteTextFormat> pToolBtnFormat;

	UniqueComPtr<IDWriteInlineObject> pTitleEllipsisTrimmingSign;
	UniqueComPtr<IDWriteInlineObject> pMainEllipsisTrimmingSign;
	UniqueComPtr<IDWriteInlineObject> pErrorMsgEllipsisTrimmingSign;
	UniqueComPtr<IDWriteInlineObject> pTipsEllipsisTrimmingSign;
	UniqueComPtr<IDWriteInlineObject> pToolBtnEllipsisTrimmingSign;

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
		return m_pDirect2dFactory.get();
	}

	IDWriteFactory* dWriteFactory() const noexcept
	{
		return m_pDWriteFactory.get();
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
		HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &m_pDirect2dFactory);
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

		constexpr DWORD pathLength = std::numeric_limits<short>::max();
		m_exeFullName.resize(pathLength);
		const DWORD resultSize = GetModuleFileNameW(nullptr, m_exeFullName.data(), pathLength);
		m_exeFullName.resize(resultSize);
		m_exeFullName = std::wstring(m_exeFullName);

		namespace fs = std::filesystem;
		const fs::path fsPath = fs::absolute(fs::path(m_exeFullName));
		m_exeDir = fsPath.parent_path().native();

		parseCmdLine();
	}

	void runMessageLoop() const
	{
		m_eventLoop.run();
	}

	void parseCmdLine() const;

private:
	HINSTANCE m_hInstance{nullptr};
	std::wstring m_strCmdLine;
	int m_nCmdShow{SW_HIDE};
	std::wstring m_exeFullName;
	std::wstring m_exeDir;

private:
	struct ComInitGuard
	{
		ComInitGuard()
		{
			if (const HRESULT hr = CoInitialize(nullptr); FAILED(hr))
			{
				throw std::runtime_error(std::format("CoInitialize fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)));
			}
		}

		~ComInitGuard()
		{
			CoUninitialize();
		}
	};

	[[no_unique_address]] ComInitGuard m_comInitGuard;
	UniqueComPtr<ID2D1Factory> m_pDirect2dFactory;
	UniqueComPtr<IDWriteFactory> m_pDWriteFactory;
	AppCommonTextFormat m_commonTextFormat;

private:
	mutable sched::EventLoopForWinUi m_eventLoop;
};

inline MainApp* g_app{nullptr};

export MainApp& app() noexcept
{
	return *g_app;
}

export void show_error_message(std::wstring_view msg)
{
	app().get_scheduler().addTask([msg = std::wstring{msg}]
	{
		MessageBoxW(nullptr, msg.c_str(), MainApp::appName.data(), MB_OK | MB_ICONERROR | MB_TASKMODAL);
	});
}

export std::wstring utf8_to_wide_string(std::string_view utf8)
{
	const int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
	if (len == 0)
	{
		throw std::runtime_error{std::format("MultiByteToWideChar fail, error code: {}", GetLastError())};
	}
	std::wstring result(len, 0);
	if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.data(), static_cast<int>(utf8.size()), result.data(), len))
	{
		throw std::runtime_error{std::format("MultiByteToWideChar fail, error code: {}", GetLastError())};
	}
	return result;
}

export void show_utf8_error_message(std::string_view msg)
{
	show_error_message(utf8_to_wide_string(msg));
}

export void show_require_elevation_message(std::wstring_view requester, std::wstring_view path);
