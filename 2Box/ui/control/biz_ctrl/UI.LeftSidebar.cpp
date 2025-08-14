module UI.LeftSidebar;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

namespace
{
	constexpr float PADDING = 24.f;
}

namespace ui
{
	void LeftSidebar::initialize()
	{
		m_startAppDiv = std::make_unique<StartAppDiv>(this);
		m_startAppDiv->setBounds(D2D1::RectF(PADDING, PADDING, PADDING + 232.f, PADDING + 36.f));
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
	}
}
