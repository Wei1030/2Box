module UI.LeftSidebar;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;

namespace
{
	constexpr float PADDING = 24.f;
	constexpr float MARGIN_BOTTOM = 24.f;
}

namespace ui
{
	void LeftSidebar::onResize(float width, float height)
	{
		resizeEnvCardsArea();
	}

	void LeftSidebar::initialize()
	{
		m_startAppDiv = std::make_unique<StartAppDiv>(this);
		m_startAppDiv->setBounds(D2D1::RectF(PADDING, PADDING, PADDING + 232.f, PADDING + 36.f));
		m_startAppDiv->setSizeChangeNotify([this](float, float)
		{
			resizeEnvCardsArea();
		});
		m_startAppDiv->setLaunchProcess([this](const std::wstring& procPath)
		{
			m_envCardsArea->launchProcess(procPath);
		});

		m_envCardsArea = std::make_unique<EnvBoxCardArea>(this);
	}

	void LeftSidebar::resizeEnvCardsArea() const
	{
		const auto contentSize = size();
		const float startYPos = m_startAppDiv->getBounds().bottom + MARGIN_BOTTOM * 2;
		m_envCardsArea->setBounds(D2D1::RectF(PADDING - EnvBoxCardArea::shadowSize,
		                                      startYPos - EnvBoxCardArea::shadowSize + EnvBoxCardArea::shadowOffsetY,
		                                      contentSize.width - PADDING + EnvBoxCardArea::scrollAreaWidth + EnvBoxCardArea::shadowSize,
		                                      contentSize.height));
	}

	void LeftSidebar::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		solidBrush->SetColor(D2D1::ColorF(0xFFFFFF));
		renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), solidBrush);

		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawLine(D2D1::Point2F(drawSize.width, 0.f), D2D1::Point2F(drawSize.width, drawSize.height), solidBrush);

		m_startAppDiv->draw(renderCtx);

		if (m_envCardsArea->isNoEnvs())
		{
			return;
		}

		const float startYPos = m_startAppDiv->getBounds().bottom + MARGIN_BOTTOM;
		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawLine(D2D1::Point2F(PADDING, startYPos), D2D1::Point2F(drawSize.width - PADDING, startYPos), solidBrush);

		m_envCardsArea->draw(renderCtx);
	}
}
