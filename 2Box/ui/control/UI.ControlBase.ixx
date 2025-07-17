export module UI.ControlBase;

import "sys_defs.h";
export import UI.WindowBase;

namespace ui
{
	export class ControlBase
	{
	public:
		virtual ~ControlBase() = default;

	public:
		void setBounds(const D2D1_RECT_F& newBounds)
		{
			m_bounds = newBounds;
		}

		D2D1_SIZE_F size() const
		{
			return D2D1::SizeF(m_bounds.right - m_bounds.left, m_bounds.bottom - m_bounds.top);
		}

	public:
		virtual WindowBase::HResult onCreateDeviceResources(const RenderContext& renderCtx)
		{
			return S_OK;
		}

		virtual void onDiscardDeviceResources()
		{
		}

		virtual void draw(const RenderContext& renderCtx)
		{
		}

	protected:
		D2D1_RECT_F m_bounds{};
	};

	export template <typename DerivedT>
	class ControlTmplBase : public ControlBase
	{
	public:
		virtual WindowBase::HResult onCreateDeviceResources(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			WindowBase::HResult hr;
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

			renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(m_bounds.left, m_bounds.top));
			static_cast<DerivedT*>(this)->drawImpl(renderCtx);
			renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

			renderTarget->PopAxisAlignedClip();
		}
	};
}
