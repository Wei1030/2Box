export module UI.Core;

import "sys_defs.h";
import std;

namespace ui
{
	export using HResult = HRESULT;
	export class WindowBase;

	export struct RenderContext
	{
		UniqueComPtr<ID2D1HwndRenderTarget> renderTarget;
		UniqueComPtr<ID2D1SolidColorBrush> brush;
	};


	export struct MouseEvent
	{
		D2D1_POINT_2F point;

		enum class ButtonType : std::uint8_t
		{
			NotInvolved,
			Left,
			Right,
			Middle,
			X1,
			X2,
		};

		ButtonType button;

		enum DownState
		{
			LeftButtonDown = 0x0001,
			RightButtonDown = 0x0002,
			ShiftDown = 0x0004,
			ControlDown = 0x0008,
			MiddleButtonDown = 0x0010,
			XButton1Down = 0x0020,
			XButton2Down = 0x0040,
		};

		std::size_t downState;
	};

	export class RendererInterface
	{
	public:
		virtual ~RendererInterface() = default;

		virtual HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget)
		{
			return S_OK;
		}

		virtual void onDiscardDeviceResources()
		{
		}

		virtual void draw(const RenderContext& renderCtx)
		{
		}
	};

	export class ControlBase : public RendererInterface
	{
	public:
		explicit ControlBase(WindowBase* owner) noexcept;
		explicit ControlBase(ControlBase* parent) noexcept;
		virtual ~ControlBase();

	public:
		WindowBase* owner() const noexcept
		{
			return m_ownerWnd;
		}

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

		void update() const;
		void updateWholeWnd() const;

	public:
		virtual HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override
		{
			HResult hr;
			do
			{
				hr = createDeviceResourcesImpl(renderTarget);
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

		virtual void draw(const RenderContext& renderCtx) override
		{
			const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;

			renderTarget->PushAxisAlignedClip(m_bounds, D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);

			D2D1_MATRIX_3X2_F oldTransform;
			renderTarget->GetTransform(&oldTransform);
			renderTarget->SetTransform(D2D1::Matrix3x2F::Translation(m_bounds.left, m_bounds.top) * oldTransform);

			drawImpl(renderCtx);

			renderTarget->SetTransform(oldTransform);

			renderTarget->PopAxisAlignedClip();
		}

	protected:
		friend class ControlManager;

		bool hitTest(D2D1_POINT_2F point) const
		{
			return point.x >= m_boundsInOwner.left && point.x <= m_boundsInOwner.right &&
				point.y >= m_boundsInOwner.top && point.y <= m_boundsInOwner.bottom;
		}

		virtual void onMouseEnter(const MouseEvent& e)
		{
		}

		virtual void onMouseLeave(const MouseEvent& e)
		{
		}

		virtual void onMouseDown(const MouseEvent& e)
		{
		}

		virtual void onMouseUp(const MouseEvent& e)
		{
		}

		virtual void onClick(const MouseEvent& e)
		{
		}

	private:
		virtual HResult createDeviceResourcesImpl(ID2D1HwndRenderTarget* renderTarget)
		{
			return S_OK;
		}

		virtual void drawImpl(const RenderContext& renderCtx)
		{
		}

	protected:
		WindowBase* m_ownerWnd;
		ControlBase* m_parent;
		D2D1_RECT_F m_bounds{};
		D2D1_RECT_F m_boundsInOwner{};
	};

	class ControlManager
	{
	public:
		ControlManager() noexcept
		{
			m_controls.reserve(20);
		}

		void addControl(ControlBase* control)
		{
			m_controls.push_back(control);
		}

		void removeControl(const ControlBase* control)
		{
			if (m_currentHovered == control)
			{
				m_currentHovered = nullptr;
			}
			if (m_currentPressed == control)
			{
				m_currentPressed = nullptr;
			}
			for (auto it = m_controls.begin(); it != m_controls.end(); ++it)
			{
				if (*it == control)
				{
					m_controls.erase(it);
					break;
				}
			}
		}

		void onMouseMove(const MouseEvent& e)
		{
			ControlBase* hovered = nullptr;

			// 从最上层开始检测
			for (auto it = m_controls.rbegin(); it != m_controls.rend(); ++it)
			{
				if ((*it)->hitTest(e.point))
				{
					hovered = *it;
					break;
				}
			}
			if (hovered != m_currentHovered)
			{
				if (m_currentHovered)
				{
					m_currentHovered->onMouseLeave(e);
				}
				if (hovered)
				{
					hovered->onMouseEnter(e);
				}
				m_currentHovered = hovered;
			}

			m_lastMousePos = e.point;
		}

		void onMouseDown(const MouseEvent& e)
		{
			if (m_currentHovered)
			{
				m_currentHovered->onMouseDown(e);
				if (e.button == MouseEvent::ButtonType::Left)
				{
					m_currentPressed = m_currentHovered;
				}
			}
		}

		void onMouseUp(const MouseEvent& e)
		{
			if (m_currentPressed)
			{
				m_currentPressed->onMouseUp(e);
				if (e.button == MouseEvent::ButtonType::Left)
				{
					if (m_currentPressed == m_currentHovered)
					{
						m_currentPressed->onClick(e);
					}
					else
					{
						m_currentPressed = nullptr;
					}
				}
			}
		}

	private:
		std::vector<ControlBase*> m_controls;
		ControlBase* m_currentHovered{nullptr};
		ControlBase* m_currentPressed{nullptr};
		D2D1_POINT_2F m_lastMousePos{};
	};

	export struct DpiInfo
	{
		float dpi{96.f};
		float physicalToDevice{1.f};
		float deviceToPhysical{1.f};
	};

	export class WindowBase : public RendererInterface
	{
	public:
		struct WindowCreateParam
		{
			std::wstring_view title;
			DWORD dwStyle = WS_OVERLAPPEDWINDOW;
			DWORD dwExStyle = 0;
			int x = CW_USEDEFAULT;
			int y = 0;
			int nWidth = CW_USEDEFAULT;
			int nHeight = 0;
			HMENU hMenu = nullptr;
		};

		explicit WindowBase(const WindowCreateParam& param = {});
		// 创建子窗口还需要做管理，如果之后有需要时再实现吧
		// explicit WindowBase(WindowBase* parentWnd, const WindowCreateParam& param = {});
	public:
		virtual ~WindowBase();

		WindowBase(const WindowBase&) = delete;
		WindowBase& operator=(const WindowBase&) = delete;
		WindowBase(WindowBase&&) = delete;
		WindowBase& operator=(WindowBase&&) = delete;

	public:
		void show(int nCmdShow = SW_SHOW) const;
		void destroyWindow();
		void setExitAppWhenWindowDestroyed(bool exit) { m_bIsExitAppWhenWindowDestroyed = exit; }
		HWND nativeHandle() const { return m_hWnd; }
		const DpiInfo& dpiInfo() const { return m_dpiInfo; }
		D2D_RECT_F rect() const;
		void setRect(const D2D_RECT_F& rect);
		D2D_RECT_F physicalRect() const;
		void setPhysicalRect(const D2D_RECT_F& rect);
		void invalidateRect(const D2D_RECT_F& rect);
		void invalidateRect();
		D2D_RECT_F rectNeedUpdate() const;

		ControlManager& controlManager() { return m_controlManager; }

	protected:
		const RenderContext& renderContext() const { return m_renderCtx; }
		void requestCreateDeviceResources();

	protected:
		virtual void onResize(std::uint32_t width, std::uint32_t height)
		{
		}

		virtual HResult onRender();

		// 返回true表示不要调用默认实现销毁窗口,而是自己处理
		// 返回false表示默认处理(直接销毁窗口)
		virtual bool onClose()
		{
			return false;
		}

	private:
		HWND createWindowInternal(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle,
		                          int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu);
		HResult prepareDeviceResources();
		void releaseDeviceResources();
		void updateDpi();
		void resize(std::uint32_t width, std::uint32_t height);
		void mouseMove(int physicalX, int physicalY, MouseEvent::ButtonType button, std::size_t downState);
		void mouseDown(int physicalX, int physicalY, MouseEvent::ButtonType button, std::size_t downState);
		void mouseUp(int physicalX, int physicalY, MouseEvent::ButtonType button, std::size_t downState);
		// 这个不做成虚函数，因为窗口有可能在基类析构中销毁，此时无法调用到子类的虚函数。索性不要这个时机了，反正有子类析构可以用
		void onDestroy();

	private:
		static void registerWndClassOnce();
		static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	private:
		HWND m_hWnd{nullptr};
		bool m_bIsExitAppWhenWindowDestroyed{false};
		DpiInfo m_dpiInfo{};
		RenderContext m_renderCtx{};
		ControlManager m_controlManager{};
	};

	ControlBase::ControlBase(WindowBase* owner) noexcept
		: m_ownerWnd(owner)
		  , m_parent(nullptr)
	{
		if (!m_ownerWnd)
		{
			std::unreachable();
		}
		m_ownerWnd->controlManager().addControl(this);
	}

	ControlBase::ControlBase(ControlBase* parent) noexcept
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
		m_ownerWnd->controlManager().addControl(this);
	}

	ControlBase::~ControlBase()
	{
		m_ownerWnd->controlManager().removeControl(this);
	}

	void ControlBase::update() const
	{
		m_ownerWnd->invalidateRect(m_boundsInOwner);
	}

	void ControlBase::updateWholeWnd() const
	{
		m_ownerWnd->invalidateRect();
	}
}
