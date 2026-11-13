module UI.LeftSidebar;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import UI.Core;
import Biz.Core;

namespace
{
	constexpr float PADDING = 24.f;
	constexpr float MARGIN_BOTTOM = 24.f;
	constexpr float START_BTN_HEIGHT = 36.f;
	constexpr float CREATE_ENV_BTN_HEIGHT = 32.f;
	constexpr float CREATE_ENV_BTN_MARGIN_TOP = 12.f;
	constexpr std::wstring_view NO_ENV_HINT_TEXT = L"暂无环境，点击下方「新建环境」";
	constexpr float NO_ENV_HINT_HEIGHT = 20.f;
}

namespace ui
{
	void LeftSidebar::onResize(float width, float height)
	{
		m_startAppDiv->setBounds(D2D1::RectF(PADDING, PADDING, width - PADDING, PADDING + m_startAppDiv->getPathAreaHeight() + START_BTN_HEIGHT));

		m_btnCreateEnv->setBounds(D2D1::RectF(PADDING, height - PADDING - CREATE_ENV_BTN_HEIGHT,
		                                      width - PADDING, height - PADDING));

		const float startYPos = m_startAppDiv->getBounds().bottom + MARGIN_BOTTOM * 2;
		const float cardsBottom = m_btnCreateEnv->getBounds().top - CREATE_ENV_BTN_MARGIN_TOP;
		m_envCardsArea->setBounds(D2D1::RectF(PADDING - EnvBoxCardArea::shadowSize,
		                                      startYPos - EnvBoxCardArea::shadowSize + EnvBoxCardArea::shadowOffsetY,
		                                      width - PADDING + EnvBoxCardArea::scrollAreaWidth + EnvBoxCardArea::shadowSize,
		                                      cardsBottom));
	}

	void LeftSidebar::initialize()
	{
		m_startAppDiv = std::make_unique<StartAppDiv>(this);
		m_startAppDiv->setBounds(D2D1::RectF(PADDING, PADDING, PADDING + 232.f, PADDING + 36.f));
		m_startAppDiv->setUpdateBounds([this]()
		{
			const auto [width, height] = size();
			onResize(width, height);
			update();
		});
		m_startAppDiv->setLaunchProcess([this](const std::wstring& procPath)
		{
			m_envCardsArea->launchProcess(procPath);
		});

		m_envCardsArea = std::make_unique<EnvBoxCardArea>(this);

		// 不启动进程直接新建环境
		m_btnCreateEnv = std::make_unique<Button>(this);
		m_btnCreateEnv->setText(L"+ 新建环境");
		m_btnCreateEnv->setBackgroundColor(D2D1::ColorF(0xffffff));
		m_btnCreateEnv->setBackgroundColor(D2D1::ColorF(0xe3f2fd), Button::EState::Hover);
		m_btnCreateEnv->setBackgroundColor(D2D1::ColorF(0xbbdefb), Button::EState::Active);
		m_btnCreateEnv->setBorderColor(D2D1::ColorF(0x0078d4));
		m_btnCreateEnv->setTextColor(D2D1::ColorF(0x0078d4));
		m_btnCreateEnv->setRadius(8.f);
		m_btnCreateEnv->setOnClick([this] { onCreateEnvBtnClick(); });
	}

	void LeftSidebar::onCreateEnvBtnClick()
	{
		try
		{
			// 仅创建环境（创建环境目录与注册表项），不启动任何进程
			biz::env_mgr().createEnv();
		}
		catch (const std::exception& e)
		{
			const std::wstring errorMsg = utf8_to_wide_string(e.what());
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"新建环境失败：{}", errorMsg).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR);
		}
	}

	void LeftSidebar::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		// 设置了背景图片时使用半透明白色底，让背景图片透出且保证文字可读
		solidBrush->SetColor(ui::has_app_background_image()
			                     ? D2D1::ColorF(0xFFFFFF, 0.7f)
			                     : D2D1::ColorF(0xFFFFFF));
		renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), solidBrush);

		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawLine(D2D1::Point2F(drawSize.width, 0.f), D2D1::Point2F(drawSize.width, drawSize.height), solidBrush);

		m_startAppDiv->draw(renderCtx);

		if (!m_envCardsArea->isNoEnvs())
		{
			const float startYPos = m_startAppDiv->getBounds().bottom + MARGIN_BOTTOM;
			solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
			renderTarget->DrawLine(D2D1::Point2F(PADDING, startYPos), D2D1::Point2F(drawSize.width - PADDING, startYPos), solidBrush);

			m_envCardsArea->draw(renderCtx);
		}
		else
		{
			// 没有环境时给出提示，避免侧边栏空白
			const D2D1_RECT_F hintRect = m_envCardsArea->getBounds();
			solidBrush->SetColor(D2D1::ColorF(0x999999));
			IDWriteTextFormat* const tipsFormat = app().textFormat().pTipsFormat;
			const DWRITE_TEXT_ALIGNMENT oldAlignment = tipsFormat->GetTextAlignment();
			tipsFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			renderTarget->DrawTextW(NO_ENV_HINT_TEXT.data(),
			                        static_cast<UINT32>(NO_ENV_HINT_TEXT.length()),
			                        tipsFormat,
			                        D2D1::RectF(hintRect.left, hintRect.top + MARGIN_BOTTOM,
			                                    hintRect.right, hintRect.top + MARGIN_BOTTOM + NO_ENV_HINT_HEIGHT),
			                        solidBrush);
			tipsFormat->SetTextAlignment(oldAlignment);
		}

		m_btnCreateEnv->draw(renderCtx);
	}
}
