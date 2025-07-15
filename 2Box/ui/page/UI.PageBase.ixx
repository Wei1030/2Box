export module UI.PageBase;

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

		virtual WindowBase::HResult onCreateDeviceResources(const RenderContext& renderCtx) { return 0; }

		virtual void onDiscardDeviceResources()
		{
		}

		virtual void draw(const RenderContext& renderCtx)
		{
		}
	};
}
