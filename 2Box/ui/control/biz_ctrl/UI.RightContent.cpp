module UI.RightContent;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

namespace
{
	constexpr float PADDING = 24.f;
	constexpr float MARGIN = 24.f;
	constexpr float FEATURES_AREA_HEIGHT = 70.f;
}

namespace ui
{
	void RightContent::onResize(float width, float height)
	{
		m_featuresArea.setBounds(D2D1::RectF(PADDING, PADDING, width - PADDING, PADDING + FEATURES_AREA_HEIGHT));
		m_processList.setBounds(D2D1::RectF(PADDING, PADDING + FEATURES_AREA_HEIGHT + MARGIN, width - PADDING, height - PADDING));
	}

	void RightContent::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		solidBrush->SetColor(D2D1::ColorF(0xf8f9fa));
		renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), solidBrush);

		m_featuresArea.draw(renderCtx);

		if (m_processList.hasEnv())
		{
			m_processList.draw(renderCtx);
		}
		else
		{
		}
	}
}
