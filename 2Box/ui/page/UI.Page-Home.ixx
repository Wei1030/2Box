export module UI.Page:Home;

import "sys_defs.h";
import std;
import UI.Core;
import UI.LeftSidebar;
import UI.RightContent;
import Biz.Core;

namespace ui
{
	export class HomePage final : public RendererInterface
	{
	public:
		static constexpr float sidebarWidth = 280.0f;

		explicit HomePage(WindowBase* owner) : m_ownerWnd(owner)
		{
			owner->addRenderer(this);

			m_leftSidebar = std::make_unique<LeftSidebar>(owner);
			m_rightContent = std::make_unique<RightContent>(owner);

			m_leftSidebar->getEnvBoxCardArea()->setOnSelect([this](const std::shared_ptr<biz::Env>& env, bool bSelected)
			{
				if (bSelected)
				{
					m_rightContent->getProcessList().setEnv(env);
				}
				else
				{
					m_rightContent->getProcessList().clearEnv();
				}
				m_rightContent->getEnvDetail().update();
			});
			m_leftSidebar->getEnvBoxCardArea()->setOnProcCountChange([this](biz::Env::EProcEvent e, const std::shared_ptr<biz::ProcessInfo>& p)
			{
				m_rightContent->getProcessList().procCountChange(e, p);
			});
		}

		virtual ~HomePage()
		{
			m_ownerWnd->removeRenderer(this);
		}

		void setMargins(const D2D1_RECT_F& margins)
		{
			m_frameMargins = margins;
		}

		virtual void onResize(float width, float height) override
		{
			m_leftSidebar->setBounds(D2D1::RectF(m_frameMargins.left, m_frameMargins.top,
			                                     m_frameMargins.left + sidebarWidth, height));
			m_rightContent->setBounds(D2D1::RectF(m_frameMargins.left + sidebarWidth, m_frameMargins.top, width, height));
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			m_rightContent->draw(renderCtx);

			draw_box_shadow(renderCtx, m_leftSidebar->getBounds(), {.offset = D2D1::Point2F(0.f, 1.f)});
			m_leftSidebar->draw(renderCtx);
		}

	private:
		WindowBase* m_ownerWnd{nullptr};
		D2D1_RECT_F m_frameMargins{};
		std::unique_ptr<LeftSidebar> m_leftSidebar;
		std::unique_ptr<RightContent> m_rightContent;
	};
}
