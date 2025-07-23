export module UI.ControlBase;

import "sys_defs.h";
import std;
export import UI.WindowBase;

namespace ui
{
	export class ControlBase
	{
	public:
		explicit ControlBase(WindowBase* owner) noexcept
			: m_ownerWnd(owner)
			  , m_parent(nullptr)
		{
			if (!m_ownerWnd)
			{
				std::unreachable();
			}
		}

		explicit ControlBase(ControlBase* parent) noexcept
			: m_parent(parent)
		{
			if (!m_parent)
			{
				std::unreachable();
			}

			m_ownerWnd = parent->m_ownerWnd;
			if (!m_ownerWnd)
			{
				std::unreachable();
			}
		}

		virtual ~ControlBase() = default;

	public:
		void setBounds(const D2D1_RECT_F& newBounds)
		{
			m_bounds = newBounds;
			m_boundsInOwner = mapToOwner(D2D1::RectF(0, 0, m_bounds.right - m_bounds.left, m_bounds.bottom - m_bounds.top));
		}

		D2D1_SIZE_F size() const
		{
			return D2D1::SizeF(m_bounds.right - m_bounds.left, m_bounds.bottom - m_bounds.top);
		}

		D2D1_RECT_F mapToParent(const D2D1_RECT_F& rect) const
		{
			return D2D1::RectF(
				rect.left + m_bounds.left,
				rect.top + m_bounds.top,
				rect.right + m_bounds.left,
				rect.bottom + m_bounds.top
			);
		}

		D2D1_RECT_F mapToOwner(const D2D1_RECT_F& rect) const
		{
			D2D1_RECT_F result = mapToParent(rect);
			const ControlBase* parent = m_parent;
			while (parent)
			{
				result = parent->mapToParent(result);
				parent = parent->m_parent;
			}
			return result;
		}

		void update() const
		{
			m_ownerWnd->invalidateRect(m_boundsInOwner);
		}

	public:
		virtual WindowBase::HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget)
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
		WindowBase* m_ownerWnd;
		ControlBase* m_parent;
		D2D1_RECT_F m_bounds{};
		D2D1_RECT_F m_boundsInOwner{};
	};

	export template <typename DerivedT>
	class ControlTmplBase : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

		virtual WindowBase::HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override
		{
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

			D2D1_MATRIX_3X2_F oldTransform;
			renderTarget->GetTransform(&oldTransform);
			renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(m_bounds.left, m_bounds.top) * oldTransform);

			static_cast<DerivedT*>(this)->drawImpl(renderCtx);

			renderTarget->SetTransform(oldTransform);

			renderTarget->PopAxisAlignedClip();
		}
	};
}
