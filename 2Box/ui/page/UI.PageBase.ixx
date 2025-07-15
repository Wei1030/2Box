export module UI.PageBase;

import "sys_defs.h";
export import UI.WindowBase;

namespace ui
{
	export class PageBase
	{
	public:
		virtual ~PageBase() = default;

		virtual HRESULT onCreateDeviceResources(const RenderContext& renderCtx) { return S_OK; }

		virtual void onDiscardDeviceResources()
		{
		}

		virtual void render(const RenderContext& renderCtx)
		{
		}
	};
}
