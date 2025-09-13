module;
#include "res/resource.h"
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
		initWindow();
		initWindowPosition();
		initTitleIcon();
		reserveRenderers(2, 20);
#if 0	// 暂时不使用反射注入，就不需要下载pdb了
		initSymbols().detachAndStart();
#else
		changePageTo<HomePage>();
#endif
	}

	HResult MainWindow::onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget)
	{
		m_pD2D1Bitmap.reset();
		if (m_bmpIconData.size())
		{
			return renderTarget->CreateBitmap(D2D1::SizeU(m_bmIcon.bmWidth, m_bmIcon.bmHeight),
			                                  m_bmpIconData.data(),
			                                  m_bmIcon.bmWidthBytes,
			                                  D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
			                                                         dpiInfo().dpi, dpiInfo().dpi),
			                                  &m_pD2D1Bitmap);
		}
		return S_OK;
	}

	void MainWindow::onDiscardDeviceResources()
	{
		m_pD2D1Bitmap.reset();
	}

	void MainWindow::draw(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;

		float captionContentHeight;
		float padding;
		if (IsMaximized(nativeHandle()))
		{
			RECT rcFrame = {};
			AdjustWindowRectEx(&rcFrame, GetWindowStyle(nativeHandle()) & ~WS_CAPTION, FALSE, GetWindowExStyle(nativeHandle()));
			const float captionHeight = m_margins.top + rcFrame.top;
			captionContentHeight = captionHeight * 0.618f;
			padding = (captionHeight - captionContentHeight) * 0.5f - rcFrame.top;
		}
		else
		{
			captionContentHeight = m_margins.top * 0.618f;
			padding = (m_margins.top - captionContentHeight) * 0.5f;
		}

		renderTarget->PushAxisAlignedClip(D2D1::RectF(0.f, 0.f, padding + captionContentHeight + padding, m_margins.top), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		renderTarget->Clear(D2D1::ColorF{0, 0.f});
		renderTarget->PopAxisAlignedClip();

		if (ID2D1Bitmap* bitmap = getTitleIconBitmap(renderTarget))
		{
			renderTarget->DrawBitmap(bitmap, D2D1::RectF(padding, padding,
			                                             padding + captionContentHeight, padding + captionContentHeight));
		}

		currentRenderer()->draw(renderCtx);
	}

	void MainWindow::onResize(float width, float height)
	{
		if (!std::holds_alternative<std::monostate>(m_pages))
		{
			currentRenderer()->onResize(width, height);
		}
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
			AdjustWindowRectEx(&rcFrame, GetWindowStyle(nativeHandle()) & ~WS_CAPTION, FALSE, GetWindowExStyle(nativeHandle()));

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
		reinitWindow();
	}

	void MainWindow::initWindow()
	{
		RECT rcFrame = {};
		AdjustWindowRectEx(&rcFrame, GetWindowStyle(nativeHandle()), FALSE, GetWindowExStyle(nativeHandle()));

		m_physicalMargins.cxLeftWidth = -rcFrame.left;
		m_physicalMargins.cxRightWidth = rcFrame.right;
		m_physicalMargins.cyTopHeight = -rcFrame.top;
		m_physicalMargins.cyBottomHeight = rcFrame.bottom;

		const float physicalToDevice = dpiInfo().physicalToDevice;
		m_margins.left = m_physicalMargins.cxLeftWidth * physicalToDevice;
		m_margins.top = m_physicalMargins.cyTopHeight * physicalToDevice;
		m_margins.right = m_physicalMargins.cxRightWidth * physicalToDevice;
		m_margins.bottom = m_physicalMargins.cyBottomHeight * physicalToDevice;

		if (isCompositionEnabled())
		{
			DWM_SYSTEMBACKDROP_TYPE t = DWMSBT_MAINWINDOW;
			DwmSetWindowAttribute(nativeHandle(), DWMWA_SYSTEMBACKDROP_TYPE, &t, sizeof(t));
			DwmExtendFrameIntoClientArea(nativeHandle(), &m_physicalMargins);
		}
	}

	void MainWindow::reinitWindow()
	{
		initWindow();
		m_pD2D1Bitmap.reset();
		if (isPage<HomePage>())
		{
			getPage<HomePage>().setMargins(m_margins);
			const D2D_RECT_F rc = rect();
			getPage<HomePage>().onResize(rc.right - rc.left, rc.bottom - rc.top);
		}
		else if (isPage<DownloadPage>())
		{
			getPage<DownloadPage>().setMargins(m_margins);
			const D2D_RECT_F rc = rect();
			getPage<HomePage>().onResize(rc.right - rc.left, rc.bottom - rc.top);
		}
		invalidateRect();
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

	void MainWindow::initTitleIcon()
	{
		HICON hIcon = LoadIconW(app().moduleInstance(), MAKEINTRESOURCE(IDI_APP_ICON));
		if (!hIcon)
		{
			return;
		}

		ICONINFO ii{};
		if (!GetIconInfo(hIcon, &ii))
		{
			return;
		}
		if (ii.hbmMask)
		{
			DeleteObject(ii.hbmMask);
		}
		if (!ii.hbmColor)
		{
			return;
		}
		HDC hdcWindow = GetDC(nativeHandle());
		do
		{
			if (!GetObjectW(ii.hbmColor, sizeof(m_bmIcon), &m_bmIcon))
			{
				break;
			}

			// 只处理BI_BITFIELDS类型， 后跟3个DWORD
			std::vector<std::byte> bmiData(sizeof(BITMAPINFOHEADER) + 3 * sizeof(RGBQUAD));
			BITMAPINFO& bmi = *reinterpret_cast<BITMAPINFO*>(bmiData.data());
			bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			if (!GetDIBits(hdcWindow, ii.hbmColor, 0, 0, nullptr, &bmi, DIB_RGB_COLORS))
			{
				break;
			}
			// 只处理BI_BITFIELDS类型
			if (bmi.bmiHeader.biCompression != BI_BITFIELDS)
			{
				break;
			}
			m_bmpIconData.resize(bmi.bmiHeader.biSizeImage);
			bmi.bmiHeader.biHeight = -bmi.bmiHeader.biHeight;
			if (!GetDIBits(hdcWindow, ii.hbmColor, 0, bmi.bmiHeader.biHeight, m_bmpIconData.data(), &bmi, DIB_RGB_COLORS))
			{
				break;
			}
			// RGBQUAD redMask = bmi.bmiColors[0];
			// RGBQUAD greenMask = bmi.bmiColors[1];  
			// RGBQUAD blueMask = bmi.bmiColors[2];
		}
		while (false);
		DeleteObject(ii.hbmColor);
		ReleaseDC(nativeHandle(), hdcWindow);
	}

	ID2D1Bitmap* MainWindow::getTitleIconBitmap(ID2D1HwndRenderTarget* renderTarget)
	{
		if (m_bmpIconData.size())
		{
			if (!m_pD2D1Bitmap)
			{
				renderTarget->CreateBitmap(D2D1::SizeU(m_bmIcon.bmWidth, m_bmIcon.bmHeight),
				                           m_bmpIconData.data(),
				                           m_bmIcon.bmWidthBytes,
				                           D2D1::BitmapProperties(D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				                                                  dpiInfo().dpi, dpiInfo().dpi),
				                           &m_pD2D1Bitmap);
			}
			return m_pD2D1Bitmap;
		}
		return nullptr;
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
