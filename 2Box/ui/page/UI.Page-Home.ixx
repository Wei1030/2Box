export module UI.Page:Home;

import "sys_defs.h";
import UI.Core;

namespace ui
{
	export class HomePage final : public RendererInterface
	{
	public:
		explicit HomePage(WindowBase* owner) : m_ownerWnd(owner)
		{
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			renderCtx.renderTarget->Clear(D2D1::ColorF(0xFFFFFF));
		}

	private:
		WindowBase* m_ownerWnd{nullptr};
	};
}
