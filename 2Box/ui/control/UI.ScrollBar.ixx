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

		bool isPressed() const { return m_isPressed; }

	protected:
		virtual void onMouseMove(const MouseEvent& e) override;
		virtual void onMouseEnter(const MouseEvent& e) override;
		virtual void onMouseLeave(const MouseEvent& e) override;
		virtual void onMouseDown(const MouseEvent& e) override;
		virtual void onMouseUp(const MouseEvent& e) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		bool m_isHovered{false};
		bool m_isPressed{false};
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

		bool isThumbPressed() const { return m_thumb.isPressed(); }

		using ThumbPosChange = std::function<void()>;
		void setThumbPosChangeNotify(ThumbPosChange fn) { m_thumbPosChangeNotify = std::move(fn); }

	protected:
		virtual void onResize(float width, float height) override;
		virtual void onMouseDown(const MouseEvent& e) override;

	private:
		void updateThumbSize();
		void updateScrollOffset();
		void adjustThumbPos();
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		ScrollBarThumb m_thumb{this};
		ThumbPosChange m_thumbPosChangeNotify;

	private:
		float m_visibleSize{0.f};
		float m_totalSize{0.f};
		float m_thumbSize{0.f};
		float m_thumbMoveFactor{1.f};
		float m_scrollOffset{0.f};
	};
}
