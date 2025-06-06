export module UI.WindowBase;

import std;
import "sys_defs.h";

namespace ui
{
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
	
	public:
		void show(int nCmdShow = SW_SHOW) const;
		void destroyWindow();
		void setExitAppWhenWindowDestroyed(bool exit){ m_bIsExitAppWhenWindowDestroyed = exit; }
		HWND getHandle() const { return m_hWnd; }

	protected:
		HRESULT prepareDeviceResources();
		void releaseDeviceResources();

	private:
		HWND createWindowInternal(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle,
		                          int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu);

	protected:
		virtual void onResize(std::uint32_t width, std::uint32_t height);
		virtual HRESULT onRender();

	private:
		// 这个不做成虚函数，因为窗口有可能在基类析构中销毁，此时无法调用到子类的虚函数。索性不要这个时机了，反正有子类析构可以用
		void onDestroy();

	private:
		static void registerWndClassOnce();
		static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	protected:
		ID2D1HwndRenderTarget* m_pRenderTarget{nullptr};

	private:
		HWND m_hWnd{nullptr};
		bool m_bIsExitAppWhenWindowDestroyed{false};
	};
}
