export module UI.PageBase;

import "sys_defs.h";

namespace ui
{
	export class PageBase
	{
	public:
		virtual ~PageBase() = default;

		virtual HRESULT onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) { return S_OK; }

		virtual void onDiscardDeviceResources()
		{
		}

		virtual HRESULT render(ID2D1HwndRenderTarget* renderTarget) { return S_OK; }
	};
}
