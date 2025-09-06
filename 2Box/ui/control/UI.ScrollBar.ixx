export module UI.ScrollBar;

import "sys_defs.h";
import std;
import UI.Core;

namespace ui
{
	export class ScrollBarThumb final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

	protected:
		// virtual void onMouseMove(const MouseEvent& e) override;
		virtual void onMouseEnter(const MouseEvent& e) override;
		virtual void onMouseLeave(const MouseEvent& e) override;
		virtual void onMouseDown(const MouseEvent& e) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		bool m_isHovered{false};
		D2D1_POINT_2F m_lastMousePos{};
	};

	export class ScrollBar final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

		void setVisibleSize(float size);
		void setTotalSize(float size);
		void scroll(float delta);
		void moveThumb(float delta);

		float getVisibleSize() const { return m_visibleSize; }
		float getScrollOffset() const { return m_scrollOffset; }

	protected:
		virtual void onResize(float width, float height) override;

	private:
		void updateThumbSize();
		void updateScrollOffset();
		void adjustThumbPos();
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		ScrollBarThumb m_thumb{this};

	private:
		float m_visibleSize{0.f};
		float m_totalSize{0.f};
		float m_thumbSize{0.f};
		float m_thumbMoveFactor{1.f};
		float m_scrollOffset{0.f};
	};
}
