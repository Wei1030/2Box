module UI.FeaturesArea;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import Biz.Core;

namespace
{
	constexpr float PADDING = 16.f;
	constexpr float GAP = 8.f;
	constexpr float BUTTON_WIDTH = 78.f;
	constexpr float BUTTON_HEIGHT = 36.f;
}

namespace ui
{
	void FeaturesArea::initialize()
	{
		m_tileWndBtn.setText(L"排布窗口");
		m_tileWndBtn.setBackgroundColor(D2D1::ColorF(0xf0f0f0), Button::EState::Normal);
		m_tileWndBtn.setBackgroundColor(D2D1::ColorF(0xe0e0e0), Button::EState::Hover);
		m_tileWndBtn.setBackgroundColor(D2D1::ColorF(0xd5d5d5), Button::EState::Active);
		m_tileWndBtn.setTextColor(D2D1::ColorF(0x333333));
		m_tileWndBtn.setOnClick([]
		{
			
		});
	}

	void FeaturesArea::onResize(float width, float height)
	{
		m_tileWndBtn.setBounds(D2D1::RectF(PADDING, PADDING, PADDING + BUTTON_WIDTH, PADDING + BUTTON_HEIGHT));
	}

	void FeaturesArea::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto [width, height] = size();

		const D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(D2D1::RectF(0.f, 0.f, width, height), 8.f, 8.f);
		solidBrush->SetColor(D2D1::ColorF(0xffffff));
		renderTarget->FillRoundedRectangle(roundedRect, solidBrush);
		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawRoundedRectangle(roundedRect, solidBrush);

		m_tileWndBtn.draw(renderCtx);
	}
}
