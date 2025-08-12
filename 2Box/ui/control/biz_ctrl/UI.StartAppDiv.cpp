module UI.StartAppDiv;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

namespace ui
{
	void StartAppDiv::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		solidBrush->SetColor(D2D1::ColorF(0xFFFFFF));
		renderTarget->FillRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), 12.f, 12.f), solidBrush);
		
		solidBrush->SetColor(D2D1::ColorF(0xe9e9e9));
		renderTarget->DrawRoundedRectangle(D2D1::RoundedRect(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), 12.f, 12.f), solidBrush);
	}
}
