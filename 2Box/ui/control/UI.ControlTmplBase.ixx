export module UI.ControlTmplBase;

import "sys_defs.h";
import UI.Core;

namespace ui
{
	export template <typename DerivedT>
	class ControlTmplBase : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

		virtual HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override
		{
			HResult hr;
			do
			{
				hr = static_cast<DerivedT*>(this)->createDeviceResourcesImpl(renderTarget);
				if (FAILED(hr))
				{
					break;
				}
			}
			while (false);

			if (FAILED(hr))
			{
				onDiscardDeviceResources();
			}
			return hr;
		}

		virtual void onDiscardDeviceResources() override
		{
			static_cast<DerivedT*>(this)->discardDeviceResourcesImpl();
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, solidBrush] = renderCtx;

			renderTarget->PushAxisAlignedClip(m_bounds, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

			D2D1_MATRIX_3X2_F oldTransform;
			renderTarget->GetTransform(&oldTransform);
			renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(m_bounds.left, m_bounds.top) * oldTransform);

			static_cast<DerivedT*>(this)->drawImpl(renderCtx);

			renderTarget->SetTransform(oldTransform);

			renderTarget->PopAxisAlignedClip();
		}
	};
}
