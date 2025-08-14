module UI.StartAppDiv;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

namespace
{
	constexpr float MARGIN = 12.f;
	constexpr float PADDING = 12.f;
}

namespace ui
{
	void StartAppDiv::initialize()
	{
		m_btnStart = std::make_unique<Button>(this);
		m_btnStart->setText(L"启动进程");

		m_btnStart->setBackgroundColor(D2D1::ColorF(0x006cbd));
		m_btnStart->setBackgroundColor(D2D1::ColorF(0x0078d4), Button::EState::Normal);
		m_btnStart->setTextColor(D2D1::ColorF(D2D1::ColorF::White));
	}

	void StartAppDiv::onResize(float width, float height)
	{
		const float btnYPos = m_pathTextHeight ? PADDING * 2.f + m_pathTextHeight + MARGIN : 0.f;
		m_btnStart->setBounds(D2D1::RectF(0, btnYPos, width, btnYPos + 36.f));
	}

	void StartAppDiv::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		m_btnStart->draw(renderCtx);
	}
}
