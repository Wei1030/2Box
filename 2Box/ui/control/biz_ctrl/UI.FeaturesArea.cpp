module UI.FeaturesArea;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

namespace ui
{
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
	}
}
