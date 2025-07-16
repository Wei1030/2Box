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
				hr = renderTarget->CreateLayer(nullptr, &m_layer);
				if (FAILED(hr))
				{
					break;
				}
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
			safe_release(&m_layer);
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			renderTarget->PushLayer(D2D1::LayerParameters(m_bounds), m_layer);

			renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(m_bounds.left, m_bounds.top));
			static_cast<DerivedT*>(this)->drawImpl(renderCtx);
			renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());

			renderTarget->PopLayer();
		}

	protected:
		ID2D1Layer* m_layer{};
	};
}
