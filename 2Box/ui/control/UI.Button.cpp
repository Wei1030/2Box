module UI.Button;

namespace ui
{
	void Button::onMouseEnter(const MouseEvent&)
	{
		m_bIsHover = true;
		if (m_painter.currentStateIndex() != EPainterType::Disabled)
		{
			m_painter.transferTo<EPainterType::Hover>();
			update();
		}
	}

	void Button::onMouseLeave(const MouseEvent& e)
	{
		m_bIsHover = false;
		if (m_painter.currentStateIndex() != EPainterType::Disabled)
		{
			m_painter.transferTo<EPainterType::Normal>();
			update();
		}
	}

	void Button::onMouseDown(const MouseEvent& e)
	{
		if (m_painter.currentStateIndex() != EPainterType::Disabled)
		{
			m_painter.transferTo<EPainterType::Active>();
			update();
		}
	}

	void Button::onMouseUp(const MouseEvent& e)
	{
		if (m_painter.currentStateIndex() != EPainterType::Disabled)
		{
			if (m_bIsHover)
			{
				m_painter.transferTo<EPainterType::Hover>();
			}
			else
			{
				m_painter.transferTo<EPainterType::Normal>();
			}
			update();
		}
	}

	void Button::onClick(const MouseEvent& e)
	{
	}

	void Button::initialize()
	{
		m_painter.transferTo<EPainterType::Normal>();
	}

	void Button::drawImpl(const RenderContext& renderCtx)
	{
		PainterContext ctx{*this, renderCtx};
		m_painter.setCtx(&ctx);
		m_painter.update();
	}

	void Button::drawNormal(const RenderContext& renderCtx, const PainterNormalStateCtx& stateCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& brush = renderCtx.brush;
		const auto drawSize = size();
		D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
			D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height),
			4.0f, 4.0f);
		brush->SetColor(D2D1::ColorF(0x0078d4));
		renderTarget->FillRoundedRectangle(roundedRect, brush);
	}

	void Button::drawHover(const RenderContext& renderCtx, const PainterHoverStateCtx& stateCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& brush = renderCtx.brush;
		const auto drawSize = size();
		D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
			D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height),
			4.0f, 4.0f);
		brush->SetColor(D2D1::ColorF(0x006cbd));
		renderTarget->FillRoundedRectangle(roundedRect, brush);
	}

	void Button::drawActive(const RenderContext& renderCtx, const PainterActiveStateCtx& stateCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& brush = renderCtx.brush;
		const auto drawSize = size();
		D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
			D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height),
			4.0f, 4.0f);
		brush->SetColor(D2D1::ColorF(0x006cbd));
		renderTarget->FillRoundedRectangle(roundedRect, brush);
	}

	void Button::drawDisabled(const RenderContext& renderCtx, const PainterDisabledStateCtx& stateCtx) const
	{
	}
}
