module;
#define NOMINMAX
#include <shobjidl_core.h>
module UI.StartAppDiv;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import Launcher;

namespace
{
	constexpr float MARGIN = 12.f;
	constexpr float PADDING = 12.f;
	constexpr float GAP = 8.f;
	constexpr float CLEAR_BTN_WIDTH = 24.f;
	constexpr float CLEAR_BTN_HEIGHT = CLEAR_BTN_WIDTH;
	constexpr float START_BTN_HEIGHT = 36.f;

	float get_path_area_height(float pathTextHeight)
	{
		return pathTextHeight == 0.f ? 0.f : PADDING * 2.f + pathTextHeight + MARGIN;
	}
}

namespace ui
{
	void StartAppDiv::initialize()
	{
		m_btnClear = std::make_unique<Button>(this);
		m_btnClear->setText(L"x");
		m_btnClear->setBackgroundColor(D2D1::ColorF(0xe0e0e0));
		m_btnClear->setBackgroundColor(D2D1::ColorF(0xf0f0f0), Button::EState::Normal);
		m_btnClear->setTextColor(D2D1::ColorF(0x333333));
		m_btnClear->setTextFormat(app().textFormat().pToolBtnFormat);
		m_btnClear->setOnClick([this]
		{
			m_strExePath.clear();
			updateBoundsWhenPathChanged();
			updateWholeWnd();
		});

		m_btnStart = std::make_unique<Button>(this);
		m_btnStart->setText(L"启动进程");
		m_btnStart->setBackgroundColor(D2D1::ColorF(0x006cbd));
		m_btnStart->setBackgroundColor(D2D1::ColorF(0x0078d4), Button::EState::Normal);
		m_btnStart->setTextColor(D2D1::ColorF(D2D1::ColorF::White));
		m_btnStart->setOnClick([this] { onBtnStartPressed(); });
	}

	void StartAppDiv::onResize(float width, float height)
	{
		const float clearBtnYPos = (PADDING * 2.f + m_pathTextHeight - CLEAR_BTN_HEIGHT) * 0.5f;
		m_btnClear->setBounds(D2D1::RectF(width - CLEAR_BTN_WIDTH - PADDING, clearBtnYPos,
		                                  width - PADDING, clearBtnYPos + CLEAR_BTN_HEIGHT));

		const float startBtnYPos = get_path_area_height(m_pathTextHeight);
		m_btnStart->setBounds(D2D1::RectF(0, startBtnYPos, width, startBtnYPos + START_BTN_HEIGHT));
	}

	void StartAppDiv::updateBoundsWhenPathChanged()
	{
		if (m_strExePath.size())
		{
			m_pathTextHeight = CLEAR_BTN_WIDTH;
			m_pExePathTextLayout.reset();
			if (SUCCEEDED(app().dWriteFactory()->CreateTextLayout(m_strExePath.c_str(),
				static_cast<UINT32>(m_strExePath.size()),
				app().textFormat().pMainFormat,
				size().width - PADDING * 2.f - CLEAR_BTN_WIDTH - GAP, std::numeric_limits<float>::max(),
				&m_pExePathTextLayout)))
			{
				m_pExePathTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_CHARACTER);
				DWRITE_TEXT_METRICS textMetrics;
				if (SUCCEEDED(m_pExePathTextLayout->GetMetrics(&textMetrics)))
				{
					m_pathTextHeight = textMetrics.height;
				}
			}
		}
		else
		{
			m_pathTextHeight = 0.f;
			m_pExePathTextLayout.reset();
		}

		const D2D1_RECT_F& rcNow = getBounds();
		setBounds(D2D1::RectF(rcNow.left, rcNow.top, rcNow.right, rcNow.top + get_path_area_height(m_pathTextHeight) + START_BTN_HEIGHT));
		update();
	}

	void StartAppDiv::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		if (m_strExePath.size())
		{
			const D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
				D2D1::RectF(0.f, 0.f, drawSize.width, PADDING * 2.f + m_pathTextHeight),
				8.0f, 8.0f);
			solidBrush->SetColor(D2D1::ColorF(0xf8f9fa));
			renderTarget->FillRoundedRectangle(roundedRect, solidBrush);
			solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
			renderTarget->DrawRoundedRectangle(roundedRect, solidBrush);

			solidBrush->SetColor(D2D1::ColorF(0x1a1a1a));
			if (m_pExePathTextLayout)
			{
				renderTarget->DrawTextLayout(D2D1::Point2F(PADDING, PADDING), m_pExePathTextLayout, solidBrush);
			}
			else
			{
				renderTarget->DrawTextW(m_strExePath.c_str(),
				                        static_cast<UINT32>(m_strExePath.length()),
				                        app().textFormat().pMainFormat,
				                        D2D1::RectF(PADDING, PADDING,
				                                    drawSize.width - PADDING - CLEAR_BTN_WIDTH - GAP, PADDING * 2 + m_pathTextHeight),
				                        solidBrush);
			}
			// clear btn
			m_btnClear->draw(renderCtx);
			solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
			renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(m_btnClear->getBounds(), 4.0f, 4.0f), solidBrush);
		}

		m_btnStart->draw(renderCtx);
	}

	void StartAppDiv::onBtnStartPressed()
	{
		if (m_strExePath.size())
		{
			biz::Launcher::instance().run(m_strExePath);
			return;
		}

		UniqueComPtr<IFileOpenDialog> fileOpen;
		HResult hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&fileOpen));
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"创建文件选择对话框失败! CoCreateInstance fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return;
		}
		COMDLG_FILTERSPEC rgSpec[] =
		{
			{L"可执行文件", L"*.exe"},
			{L"所有文件", L"*.*"}
		};
		fileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
		fileOpen->SetFileTypeIndex(1);
		DWORD dwOptions = 0;
		fileOpen->GetOptions(&dwOptions);
		fileOpen->SetOptions(dwOptions | FOS_STRICTFILETYPES | FOS_FORCEFILESYSTEM);
		hr = fileOpen->Show(m_ownerWnd->nativeHandle());
		if (FAILED(hr))
		{
			if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
			{
				MessageBoxW(m_ownerWnd->nativeHandle(),
				            std::format(L"显示文件选择对话框失败! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
				            MainApp::appName.data(),
				            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			}
			return;
		}
		UniqueComPtr<IShellItem> item;
		hr = fileOpen->GetResult(&item);
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"无法获取选择的文件! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return;
		}
		PWSTR pszFilePath;
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"无法获取选择的文件路径! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return;
		}
		m_strExePath = pszFilePath;
		CoTaskMemFree(pszFilePath);

		updateBoundsWhenPathChanged();

		biz::Launcher::instance().run(m_strExePath);
	}
}
