module UI.MainWindow;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import std;
import MainApp;
import Scheduler;

namespace ui
{
	static constexpr float DESIRED_WIDTH = 1024.f;
	static constexpr float DESIRED_HEIGHT = 768.f;

	MainWindow::MainWindow() : WindowBase({MainApp::appName})
	{
		setExitAppWhenWindowDestroyed(true);
		initWindowInfo();
		initWindowPosition();
		reserveRenderers(2, 20);
#if 0	// 暂时不使用反射注入，就不需要下载pdb了
		initSymbols().detachAndStart();
#else
		changePageTo<HomePage>();
#endif
	}

	void MainWindow::draw(const RenderContext& renderCtx)
	{
		currentRenderer()->draw(renderCtx);
	}

	void MainWindow::onResize(float width, float height)
	{
		if (!std::holds_alternative<std::monostate>(m_pages))
		{
			currentRenderer()->onResize(width, height);
		}
	}

	void MainWindow::onActivate(WParam wParam, LParam lParam)
	{
		extendFrameIfCompositionEnabled();
	}

	bool MainWindow::onClose()
	{
		if (isPage<DownloadPage>())
		{
			const DownloadPage& downloadPage = getPage<DownloadPage>();
			if (!downloadPage.isCancelled())
			{
				downloadPage.cancelTask();
				[](MainWindow& self, const DownloadPage& page) -> coro::OnewayTask
				{
					co_await page.joinAsync();
					self.destroyWindow();
				}(*this, downloadPage);
			}
			return true;
		}
		return false;
	}

	bool MainWindow::onNcCalcSize(WParam wParam, LParam lParam)
	{
		if (isCompositionEnabled())
		{
			if (wParam)
			{
				return true;
			}
		}
		return false;
	}

	LResult MainWindow::onNcHitTest(WPARAM wParam, LParam lParam, LResult dwmProcessedResult)
	{
		// Get the point coordinates for the hit test.
		POINT ptMouse = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

		if (isCompositionEnabled())
		{
			if (dwmProcessedResult != HTNOWHERE)
			{
				return dwmProcessedResult;
			}

			// Get the window rectangle.
			RECT rcWindow;
			GetWindowRect(nativeHandle(), &rcWindow);

			// Get the frame rectangle, adjusted for the style without a caption.
			RECT rcFrame = {};
			AdjustWindowRectEx(&rcFrame, WS_OVERLAPPEDWINDOW & ~WS_CAPTION, FALSE, NULL);

			// Determine if the hit test is for resizing. Default middle (1,1).
			USHORT uRow = 1;
			USHORT uCol = 1;
			bool fOnResizeBorder = false;

			// Determine if the point is at the top or bottom of the window.
			if (ptMouse.y >= rcWindow.top && ptMouse.y < rcWindow.top + m_physicalMargins.cyTopHeight)
			{
				fOnResizeBorder = (ptMouse.y < (rcWindow.top - rcFrame.top));
				uRow = 0;
			}
			else if (ptMouse.y < rcWindow.bottom && ptMouse.y >= rcWindow.bottom - m_physicalMargins.cyBottomHeight)
			{
				uRow = 2;
			}

			// Determine if the point is at the left or right of the window.
			if (ptMouse.x >= rcWindow.left && ptMouse.x < rcWindow.left + m_physicalMargins.cxLeftWidth)
			{
				uCol = 0; // left side
			}
			else if (ptMouse.x < rcWindow.right && ptMouse.x >= rcWindow.right - m_physicalMargins.cxRightWidth)
			{
				uCol = 2; // right side
			}

			// Hit test (HTTOPLEFT, ... HTBOTTOMRIGHT)
			LRESULT hitTests[3][3] =
			{
				{HTTOPLEFT, fOnResizeBorder ? HTTOP : HTCAPTION, HTTOPRIGHT},
				{HTLEFT, HTNOWHERE, HTRIGHT},
				{HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT},
			};

			return hitTests[uRow][uCol];
		}
		return HTNOWHERE;
	}

	void MainWindow::onNcPaint(WParam wParam, LParam lParam)
	{
	}

	void MainWindow::onDwmCompositionChanged()
	{
		extendFrameIfCompositionEnabled();
	}

	void MainWindow::initWindowInfo()
	{
		WINDOWINFO info{sizeof(WINDOWINFO)};
		if (!GetWindowInfo(nativeHandle(), &info))
		{
			return;
		}
		m_physicalMargins.cxLeftWidth = info.rcClient.left - info.rcWindow.left;
		m_physicalMargins.cxRightWidth = info.rcWindow.right - info.rcClient.right;
		m_physicalMargins.cyTopHeight = info.rcClient.top - info.rcWindow.top;
		m_physicalMargins.cyBottomHeight = info.rcWindow.bottom - info.rcClient.bottom;

		const float physicalToDevice = dpiInfo().physicalToDevice;
		m_margins.left = m_physicalMargins.cxLeftWidth * physicalToDevice;
		m_margins.top = m_physicalMargins.cyTopHeight * physicalToDevice;
		m_margins.right = m_physicalMargins.cxRightWidth * physicalToDevice;
		m_margins.bottom = m_physicalMargins.cyBottomHeight * physicalToDevice;
	}

	void MainWindow::initWindowPosition()
	{
		const auto rc = rect();
		const float width = rc.right - rc.left;
		const float height = rc.bottom - rc.top;

		const float desiredWidth = std::min(DESIRED_WIDTH, width);
		const float desiredHeight = std::min(DESIRED_HEIGHT, height);

		const float diffWidth = width - desiredWidth;
		const float diffHeight = height - desiredHeight;

		const float desiredX = rc.left + diffWidth * 0.5f;
		const float desiredY = rc.top + diffHeight * 0.5f;

		setRect(D2D1::RectF(
			        desiredX,
			        desiredY,
			        desiredX + desiredWidth,
			        desiredY + desiredHeight
		        ), SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
	}

	void MainWindow::extendFrameIfCompositionEnabled() const
	{
		if (isCompositionEnabled())
		{
			DwmExtendFrameIntoClientArea(nativeHandle(), &m_physicalMargins);
		}
	}

	coro::LazyTask<void> MainWindow::initSymbols()
	{
		changePageTo<DownloadPage>();

		co_await getPage<DownloadPage>().untilSuccess();

		if (getPage<DownloadPage>().isFileVerified())
		{
			changePageTo<HomePage>();
		}
		co_return;
	}
}
