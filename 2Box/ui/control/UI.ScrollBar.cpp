module UI.ScrollBar;

namespace ui
{
	void ScrollBar::setVisibleSize(float size)
	{
		m_visibleSize = size;
		updateSize();
	}

	void ScrollBar::setTotalSize(float size)
	{
		m_totalSize = size;
		updateSize();
	}

	void ScrollBar::scroll(float delta)
	{
		m_thumbOffset += delta;
		updateSize();
	}

	void ScrollBar::updateSize()
	{
		m_thumbOffset = std::max(0.f, std::min(m_thumbOffset, m_totalSize - m_visibleSize));

		m_thumbSize = m_visibleSize - m_thumbOffset;
	}
}
