module UI.ScrollBar;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

namespace ui
{
	// void ScrollBarThumb::onMouseMove(const MouseEvent& e)
	// {
	// 	const float yDelta = e.point.y - m_lastMousePos.y;
	// 	ScrollBar* bar = static_cast<ScrollBar*>(parent());
	// 	bar->moveThumb(yDelta);
	//
	// 	m_lastMousePos = e.point;
	// }

	void ScrollBarThumb::onMouseEnter(const MouseEvent& e)
	{
		m_isHovered = true;
		update();
	}

	void ScrollBarThumb::onMouseLeave(const MouseEvent& e)
	{
		m_isHovered = false;
		update();
	}

	void ScrollBarThumb::onMouseDown(const MouseEvent& e)
	{
		m_lastMousePos = e.point;

		e.accept = false;
	}

	void ScrollBarThumb::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();
		if (m_isHovered)
		{
			solidBrush->SetColor(D2D1::ColorF(0xe8e8e8));
		}
		else
		{
			solidBrush->SetColor(D2D1::ColorF(0xf0f0f0));
		}
		renderTarget->FillRoundedRectangle(D2D1::RoundedRect(
			                                   D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height),
			                                   4.f, 4.f),
		                                   solidBrush);
	}

	void ScrollBar::setVisibleSize(float size)
	{
		m_visibleSize = size;
		updateScrollOffset();
		updateThumbSize();
		adjustThumbPos();
	}

	void ScrollBar::setTotalSize(float size)
	{
		m_totalSize = size;
		updateScrollOffset();
		updateThumbSize();
		adjustThumbPos();
	}

	void ScrollBar::scroll(float delta)
	{
		m_scrollOffset += delta;
		updateScrollOffset();
		adjustThumbPos();
	}

	void ScrollBar::moveThumb(float delta)
	{
		if (m_thumbMoveFactor > 0.001f)
		{
			scroll(delta / m_thumbMoveFactor);
		}
	}

	void ScrollBar::onResize(float width, float height)
	{
		adjustThumbPos();
	}

	void ScrollBar::updateThumbSize()
	{
		if (m_totalSize <= m_visibleSize || m_visibleSize < 5.f)
		{
			m_thumbSize = 0.f;
			return;
		}

		const float maxScrollOffset = m_totalSize - m_visibleSize;
		const float minThumbSize = std::min(m_visibleSize * 0.5f, 24.f);
		const float maxThumbOffset = m_visibleSize - minThumbSize;
		if (maxScrollOffset <= maxThumbOffset)
		{
			m_thumbSize = m_visibleSize - maxScrollOffset;
			m_thumbMoveFactor = 1.f;
		}
		else
		{
			m_thumbSize = minThumbSize;
			m_thumbMoveFactor = maxThumbOffset / maxScrollOffset;
		}
	}

	void ScrollBar::updateScrollOffset()
	{
		m_scrollOffset = std::max(0.f, std::min(m_scrollOffset, m_totalSize - m_visibleSize));
	}

	void ScrollBar::adjustThumbPos()
	{
		const auto [width , height] = size();
		const float thumbOffset = m_scrollOffset * m_thumbMoveFactor;
		m_thumb.setBounds(D2D1::RectF(0.f, thumbOffset, width, thumbOffset + m_thumbSize));
	}

	void ScrollBar::drawImpl(const RenderContext& renderCtx)
	{
		if (m_thumbSize < 1.f)
		{
			return;
		}
		m_thumb.draw(renderCtx);
	}
}
