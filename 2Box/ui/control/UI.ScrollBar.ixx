export module UI.ScrollBar;

import std;
import UI.Core;

namespace ui
{
	export class ScrollBar final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

		void setVisibleSize(float size);
		void setTotalSize(float size);
		void scroll(float delta);

		float getVisibleSize() const { return m_visibleSize; }
		float getThumbOffset() const { return m_thumbOffset; }

	private:
		void updateSize();

	private:
		float m_visibleSize{0.f};
		float m_totalSize{0.f};
		float m_thumbSize{0.f};
		float m_thumbOffset{0.f};
	};
}
