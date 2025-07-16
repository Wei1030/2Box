export module UI.WindowBase;

import std;
import "sys_defs.h";

namespace ui
{
	export struct DpiInfo
	{
		float dpi{96.f};
		float physicalToDevice{1.f};
		float deviceToPhysical{1.f};
	};

	export struct RenderContext
	{
		RenderContext() = default;
		RenderContext(const RenderContext& rhs) = delete;
		RenderContext& operator=(const RenderContext& rhs) = delete;
		RenderContext(RenderContext&& rhs) = delete;
		RenderContext& operator=(RenderContext&& rhs) = delete;

		~RenderContext()
		{
			safe_release(&renderTarget);
			safe_release(&brush);
		}

		ID2D1HwndRenderTarget* renderTarget{nullptr};
		ID2D1SolidColorBrush* brush{nullptr};
	};

	export struct RectChangeContext
	{
		D2D_RECT_U physicalRc;
		D2D_RECT_F deviceRc;
	};

	export class WindowBase
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

		using HResult = HRESULT;

	public:
		void show(int nCmdShow = SW_SHOW) const;
		void destroyWindow();
		void setExitAppWhenWindowDestroyed(bool exit) { m_bIsExitAppWhenWindowDestroyed = exit; }
		HWND nativeHandle() const { return m_hWnd; }
		const DpiInfo& dpiInfo() const { return m_dpiInfo; }
		D2D_RECT_F physicalRect() const;
		void setPhysicalRect(const D2D_RECT_F& rect);

	protected:
		HRESULT prepareDeviceResources();
		void releaseDeviceResources();
		const RenderContext& renderContext() const { return m_renderCtx; }

	private:
		HWND createWindowInternal(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle,
		                          int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu);

	protected:
		virtual void onResize(const RectChangeContext& ctx)
		{
		}

		virtual HResult onCreateDeviceResources()
		{
			return S_OK;
		}

		virtual void onDiscardDeviceResources()
		{
		}

		virtual HResult onRender();

	private:
		void updateDpi();
		void resize(std::uint32_t width, std::uint32_t height);
		// 这个不做成虚函数，因为窗口有可能在基类析构中销毁，此时无法调用到子类的虚函数。索性不要这个时机了，反正有子类析构可以用
		void onDestroy();

	private:
		static void registerWndClassOnce();
		static LRESULT CALLBACK wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	protected:
		DpiInfo m_dpiInfo{};
		RenderContext m_renderCtx{};

	private:
		HWND m_hWnd{nullptr};
		bool m_bIsExitAppWhenWindowDestroyed{false};
	};
}
