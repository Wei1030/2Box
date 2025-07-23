export module UI.PageBase;

import "sys_defs.h";
export import UI.WindowBase;

namespace ui
{
	export class PageBase
	{
	public:
		virtual ~PageBase() = default;

		virtual void onResize(const RectChangeContext& ctx)
		{
		}

		virtual WindowBase::HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) { return 0; }

		virtual void onDiscardDeviceResources()
		{
		}

		virtual void draw(const RenderContext& renderCtx)
		{
		}
	};
}
