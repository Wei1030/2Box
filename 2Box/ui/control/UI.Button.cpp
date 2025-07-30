module UI.Button;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;

namespace ui
{
	void Button::setTextFormat(IDWriteTextFormat* textFormat, EState state)
	{
		if (state == EState::All)
		{
			applyAllState(std::make_index_sequence<static_cast<std::size_t>(EState::All)>(), &Button::setTextFormat, this, textFormat);
			return;
		}
		button_detail::PainterBase& ctx = ctxFromState(state);
		ctx.textFormat = textFormat;
		if (ctx.textLayout && ctx.text.size())
		{
			ctx.textLayout.reset();
			app().dWriteFactory()->CreateTextLayout(ctx.text.data(),
			                                        static_cast<UINT32>(ctx.text.size()),
			                                        textFormat,
			                                        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
			                                        &ctx.textLayout);
		}
	}

	void Button::setText(std::wstring_view text, EState state)
	{
		if (state == EState::All)
		{
			applyAllState(std::make_index_sequence<static_cast<std::size_t>(EState::All)>(), &Button::setText, this, text);
			return;
		}
		button_detail::PainterBase& ctx = ctxFromState(state);
		ctx.text = text;
		ctx.textLayout.reset();
		IDWriteTextFormat* textFormat = ctx.textFormat ? ctx.textFormat : app().textFormat().pMainFormat;
		app().dWriteFactory()->CreateTextLayout(text.data(),
		                                        static_cast<UINT32>(text.size()),
		                                        textFormat,
		                                        std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
		                                        &ctx.textLayout);
	}

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
		if (e.button != MouseEvent::ButtonType::Left)
		{
			return;
		}
		if (m_painter.currentStateIndex() != EPainterType::Disabled)
		{
			m_painter.transferTo<EPainterType::Active>();
			update();
		}
	}

	void Button::onMouseUp(const MouseEvent& e)
	{
		if (e.button != MouseEvent::ButtonType::Left)
		{
			return;
		}
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

	void Button::drawByState(const RenderContext& renderCtx, const PainterStateCtx& stateCtx) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& brush = renderCtx.brush;
		const auto drawSize = size();
		const D2D1_ROUNDED_RECT roundedRect = D2D1::RoundedRect(
			D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height),
			4.0f, 4.0f);
		brush->SetColor(stateCtx.backgroundColor);
		renderTarget->FillRoundedRectangle(roundedRect, brush);

		brush->SetColor(stateCtx.textColor);
		if (stateCtx.textLayout)
		{
			stateCtx.textLayout->SetMaxWidth(drawSize.width);
			stateCtx.textLayout->SetMaxHeight(drawSize.height);
			float textWidth = drawSize.width;
			float textHeight = drawSize.height;
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(stateCtx.textLayout->GetMetrics(&textMetrics)))
			{
				textWidth = textMetrics.width;
				textHeight = textMetrics.height;
			}
			const float xPos = (drawSize.width - textWidth) * 0.5f;
			const float yPos = (drawSize.height - textHeight) * 0.5f;
			renderTarget->DrawTextLayout(D2D1::Point2F(xPos, yPos), stateCtx.textLayout, brush);
		}
		else
		{
			IDWriteTextFormat* textFormat = stateCtx.textFormat ? stateCtx.textFormat : app().textFormat().pMainFormat;
			renderTarget->DrawTextW(stateCtx.text.c_str(), static_cast<UINT32>(stateCtx.text.length()), textFormat,
			                        D2D1::RectF(0, 0, drawSize.width, drawSize.height), brush);
		}
	}
}
