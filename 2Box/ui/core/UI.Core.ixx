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
			for (auto it = m_controls.begin(); it != m_controls.end(); ++it)
			{
				if (*it == control)
				{
					m_controls.erase(it);
					break;
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
