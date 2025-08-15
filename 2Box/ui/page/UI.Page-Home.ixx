export module UI.Page:Home;

import "sys_defs.h";
import std;
import UI.Core;
import UI.LeftSidebar;
import UI.RightContent;

namespace ui
{
	export class HomePage final : public RendererInterface
	{
	public:
		static constexpr float sidebarWidth = 280.0f;
		static constexpr float sidebarShadowBlur = 3.f;

		explicit HomePage(WindowBase* owner) : m_ownerWnd(owner)
		{
			owner->addRenderer(this);

			m_leftSidebar = std::make_unique<LeftSidebar>(owner);
			m_rightContent = std::make_unique<RightContent>(owner);

			const D2D_RECT_F ownerRc = owner->rect();
			m_leftSidebar->setBounds(D2D1::RectF(0.f, 0.f, sidebarWidth, ownerRc.bottom));
			m_rightContent->setBounds(D2D1::RectF(sidebarWidth, 0, ownerRc.right, ownerRc.bottom));
		}

		virtual ~HomePage()
		{
			m_ownerWnd->removeRenderer(this);
		}

		virtual void onResize(float width, float height) override
		{
			m_leftSidebar->setBounds(D2D1::RectF(0.f, 0.f, sidebarWidth, height));
			m_rightContent->setBounds(D2D1::RectF(sidebarWidth, 0, width, height));
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			m_rightContent->draw(renderCtx);
			
			draw_box_shadow(renderCtx, m_leftSidebar->getBounds(), {.offset = D2D1::Point2F(0.f, 1.f)});
			m_leftSidebar->draw(renderCtx);
		}

	private:
		WindowBase* m_ownerWnd{nullptr};
		std::unique_ptr<LeftSidebar> m_leftSidebar;
		std::unique_ptr<RightContent> m_rightContent;
	};
}
