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

	static constexpr UINT TRAY_ID = 1;
	static constexpr UINT TRAY_MESSAGE = WM_USER + 9527;

	MainWindow::MainWindow() : WindowBase({MainApp::appName})
	{
		setExitAppWhenWindowDestroyed(true);
		initWindow();
		initWindowPosition();
		initTitleIcon();
		initTray();
		reserveRenderers(2, 20);

		m_btnToTray.setBackgroundColor(D2D1::ColorF(0, 0.f), Button::EState::Normal);
		m_btnToTray.setBackgroundColor(D2D1::ColorF(0, 0.102f), Button::EState::Hover);
		m_btnToTray.setBackgroundColor(D2D1::ColorF(0, 0.208f), Button::EState::Active);
		m_btnToTray.setOnClick([this]
		{
			show(SW_HIDE);
		});
		m_btnToTray.setDrawCallback(std::bind(&MainWindow::drawToTryBtn, this, std::placeholders::_1, std::placeholders::_2));

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
		// 绘制标题栏
		if (isCompositionEnabled())
		{
			const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
			const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
			const D2D1_RECT_F rc = rect();
			const float width = rc.right - rc.left;
			renderTarget->PushAxisAlignedClip(D2D1::RectF(0.f, 0.f, width - m_captionBtnWidth, m_margins.top), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
			renderTarget->Clear(D2D1::ColorF{0, 0.f});
			constexpr float toTrayBthMarginRight = 0.f;
			const float toTrayBthWidth = m_captionBtnWidth / 3.f;
			const float toTrayBthXPos = width - m_captionBtnWidth - toTrayBthMarginRight - toTrayBthWidth;

			float paddingTop{0};
			float paddingLeft{8.f};
			if (IsMaximized(nativeHandle()))
			{
				RECT rcFrame = {};
				AdjustWindowRectEx(&rcFrame, GetWindowStyle(nativeHandle()) & ~WS_CAPTION, FALSE, GetWindowExStyle(nativeHandle()));
				paddingTop -= rcFrame.top;
				paddingLeft -= rcFrame.left;
			}
			const float captionHeight = m_margins.top - paddingTop;
			const float titleIconSize = captionHeight * 0.618f;

			if (ID2D1Bitmap* bitmap = getTitleIconBitmap(renderTarget))
			{
				const float yPos = (captionHeight - titleIconSize) * 0.5f + paddingTop;
				renderTarget->DrawBitmap(bitmap, D2D1::RectF(paddingLeft, yPos,
				                                             paddingLeft + titleIconSize, yPos + titleIconSize));
			}
			const float textXPos = paddingLeft + titleIconSize + 8.f;
			const float textYPos = (captionHeight - m_titleTextHeight) * 0.5f + paddingTop - 1.f;
			solidBrush->SetColor(m_titleColor);
			if (m_pTitleLayout)
			{
				renderTarget->DrawTextLayout(D2D1::Point2F(textXPos, textYPos), m_pTitleLayout, solidBrush);
			}
			else
			{
				renderTarget->DrawTextW(MainApp::appName.data(),
				                        static_cast<UINT32>(MainApp::appName.length()),
				                        app().textFormat().pMainFormat,
				                        D2D1::RectF(textXPos, textYPos, toTrayBthXPos, textYPos + m_titleTextHeight),
				                        solidBrush);
			}
			m_btnToTray.setBounds(D2D1::Rect(toTrayBthXPos, paddingTop + 1.f, toTrayBthXPos + toTrayBthWidth, m_margins.top));
			m_btnToTray.draw(renderCtx);
			renderTarget->PopAxisAlignedClip();
		}

		currentRenderer()->draw(renderCtx);
	}

	void MainWindow::drawToTryBtn(const RenderContext& renderCtx, Button::EState) const
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const D2D1_RECT_F& bounds = m_btnToTray.getBounds();
		const float width = bounds.right - bounds.left;
		const float height = bounds.bottom - bounds.top;
		const float contentWidth = width * 0.236f;
		const float contentHeight = height * 0.382f;
		const float paddingLr = width * 0.382f;
		const float paddingTb = height * 0.309f;
		const float contentTopHeight = contentHeight * 0.618f;
		const float contentBottomYPos = paddingTb + contentTopHeight + 0.191f * contentHeight;

		solidBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
		const D2D1_POINT_2F pt1{D2D1::Point2F(paddingLr, paddingTb)};
		const D2D1_POINT_2F pt2{D2D1::Point2F(paddingLr + contentWidth, paddingTb)};
		const D2D1_POINT_2F pt3{D2D1::Point2F((pt1.x + pt2.x) * 0.5f, paddingTb + contentTopHeight)};
		//renderTarget->DrawLine(pt1, pt2, solidBrush, 0.5f);
		renderTarget->DrawLine(pt2, pt3, solidBrush, 0.5f);
		renderTarget->DrawLine(pt3, pt1, solidBrush, 0.5f);

		renderTarget->DrawLine(D2D1::Point2F(pt1.x, contentBottomYPos), D2D1::Point2F(pt2.x, contentBottomYPos), solidBrush, 0.5f);
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
		if (LOWORD(wParam) != WA_INACTIVE)
		{
			if (isCompositionEnabled())
			{
				RECT rc{};
				DwmGetWindowAttribute(nativeHandle(), DWMWA_CAPTION_BUTTON_BOUNDS, &rc, sizeof(rc));
				m_captionBtnWidth = (rc.right - rc.left) * dpiInfo().physicalToDevice;
			}
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
					self.destroyTray();
					self.destroyWindow();
				}(*this, downloadPage);
			}
			return true;
		}
		destroyTray();
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
			if (ncBtnHitTest(ptMouse))
			{
				return HTCLIENT;
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

	void MainWindow::onUserMsg(UINT message, WParam wParam, LParam lParam)
	{
		if (message == TRAY_MESSAGE)
		{
			if (lParam == WM_LBUTTONUP)
			{
				show(SW_RESTORE);
				SetForegroundWindow(nativeHandle());
			}
			else if (lParam == WM_RBUTTONUP)
			{
			}
		}
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

		m_titleColor = D2D1::ColorF{static_cast<unsigned int>(GetSystemMetrics(COLOR_CAPTIONTEXT))};
		m_titleTextHeight = 20.f;
		m_pTitleLayout.reset();
		if (SUCCEEDED(app().dWriteFactory()->CreateTextLayout(MainApp::appName.data(),
			static_cast<UINT32>(MainApp::appName.size()),
			app().textFormat().pMainFormat,
			std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
			&m_pTitleLayout)))
		{
			DWRITE_TEXT_METRICS textMetrics;
			if (SUCCEEDED(m_pTitleLayout->GetMetrics(&textMetrics)))
			{
				m_titleTextHeight = textMetrics.height;
			}
		}

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
		m_hIcon = LoadIconW(app().moduleInstance(), MAKEINTRESOURCE(IDI_APP_ICON));
		if (!m_hIcon)
		{
			return;
		}

		ICONINFO ii{};
		if (!GetIconInfo(m_hIcon, &ii))
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

	void MainWindow::initTray() const
	{
		NOTIFYICONDATAW nid = {sizeof(nid)};
		nid.hWnd = nativeHandle();
		nid.uID = TRAY_ID;
		nid.uFlags = NIF_ICON | NIF_MESSAGE;
		nid.uCallbackMessage = TRAY_MESSAGE;
		nid.hIcon = m_hIcon;

		Shell_NotifyIconW(NIM_ADD, &nid);
	}

	void MainWindow::destroyTray() const
	{
		NOTIFYICONDATAW nid = {sizeof(nid)};
		nid.hWnd = nativeHandle();
		nid.uID = TRAY_ID;
		Shell_NotifyIconW(NIM_DELETE, &nid);
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

	bool MainWindow::ncBtnHitTest(POINT pt) const
	{
		//这里的入参pt是相对于屏幕的
		ScreenToClient(nativeHandle(), &pt);
		return m_btnToTray.hitTest(D2D1::Point2F(pt.x * dpiInfo().physicalToDevice, pt.y * dpiInfo().physicalToDevice));
	}
}
