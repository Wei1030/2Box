module;
#include "res/resource.h"
#define WM_COPYGLOBALDATA 0x0049
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
		DragAcceptFiles(nativeHandle(), TRUE);
		ChangeWindowMessageFilterEx(nativeHandle(), WM_DROPFILES, MSGFLT_ALLOW, nullptr);
		ChangeWindowMessageFilterEx(nativeHandle(), WM_COPYGLOBALDATA, MSGFLT_ALLOW, nullptr);

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

	MainWindow::~MainWindow()
	{
		// 引起MainWindow析构，也即导致main函数中的ui::MainWindow mainWnd析构的情况：
		// 1、程序式的主动调用destroyWindow, 引起的app().exit()退出消息循环。
		//		这种情况nativeHandle为空就不能也不需要调用destroyTray了, 因为主动调用destroyWindow引发的onBeforeWindowDestroy中已经destroyTray了
		// 2、用户任何形式的点击关闭，触发onClose且未进行阻止时，会先触发一次onBeforeWindowDestroy进行destroyTray，再由系统调用::DestroyWindow引起的app().exit()退出消息循环.
		//		这种情况和1一样，nativeHandle为空，且已经处理过destroyTray
		// 3、在没有销毁窗口的情况下，直接由于app().exit()而析构，这种情况下父类析构由于无法使用虚函数，子类必须自己在析构中处理额外清理业务。
		//		这种情况nativeHandle还有效，可以且必须destroyTray
		if (nativeHandle())
		{
			destroyTray();
		}
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
			solidBrush->SetColor(D2D1::ColorF{0});
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
		const D2D1_RECT_F& bounds = m_btnToTray.getBounds();

		if (isCompositionEnabled())
		{
			const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
			const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
			const float width = bounds.right - bounds.left;
			const float height = bounds.bottom - bounds.top;
			const float contentWidth = width * 0.236f;
			const float contentHeight = height * 0.382f;
			const float paddingLr = width * 0.382f;
			const float paddingTb = height * 0.309f;
			const float contentTopHeight = contentHeight * 0.618f;
			const float contentBottomYPos = paddingTb + contentTopHeight + 0.191f * contentHeight;
			const D2D1_POINT_2F pt1{D2D1::Point2F(paddingLr, paddingTb)};
			const D2D1_POINT_2F pt2{D2D1::Point2F(paddingLr + contentWidth, paddingTb)};
			const D2D1_POINT_2F pt3{D2D1::Point2F((pt1.x + pt2.x) * 0.5f, paddingTb + contentTopHeight)};

			solidBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
			//renderTarget->DrawLine(pt1, pt2, solidBrush, 0.5f);
			renderTarget->DrawLine(pt2, pt3, solidBrush, 0.5f);
			renderTarget->DrawLine(pt3, pt1, solidBrush, 0.5f);

			renderTarget->DrawLine(D2D1::Point2F(pt1.x, contentBottomYPos), D2D1::Point2F(pt2.x, contentBottomYPos), solidBrush, 0.5f);
		}
		else
		{
			HDC hdc = GetWindowDC(nativeHandle());
			const float deviceToPhysical = dpiInfo().deviceToPhysical;
			// 先以设备单位转到非客户区坐标系再转到逻辑单位
			D2D1_RECT_F physicalBounds = D2D1::RectF((bounds.left + m_margins.left) * deviceToPhysical,
			                                         (bounds.top + m_margins.top) * deviceToPhysical,
			                                         (bounds.right + m_margins.left) * deviceToPhysical,
			                                         (bounds.bottom + m_margins.top) * deviceToPhysical);
			const LONG width = static_cast<LONG>(physicalBounds.right - physicalBounds.left);
			const LONG height = static_cast<LONG>(physicalBounds.bottom - physicalBounds.top);
			const LONG contentWidth = static_cast<LONG>(width * 0.382f);
			// const LONG contentHeight = static_cast<LONG>(height * 0.382f);
			const LONG paddingLr = static_cast<LONG>(width * 0.309f);
			// const LONG paddingTb = static_cast<LONG>(height * 0.309f);
			// const LONG contentTopHeight = static_cast<LONG>(contentHeight * 0.618f);

			const RECT rcFill{static_cast<LONG>(physicalBounds.left), static_cast<LONG>(physicalBounds.top), static_cast<LONG>(physicalBounds.right), static_cast<LONG>(physicalBounds.bottom)};
			HBRUSH hbr = CreateSolidBrush(RGB(214, 211, 206));
			FillRect(hdc, &rcFill, hbr);
			DeleteObject(hbr);

			HPEN pen = CreatePen(PS_SOLID, 2, RGB(0, 0, 0));
			HPEN oldPen = static_cast<HPEN>(SelectObject(hdc, pen));
			Rectangle(hdc, rcFill.left, rcFill.top, rcFill.right, rcFill.bottom);

			// const D2D1_POINT_2U pt1{D2D1::Point2U(rcFill.left + paddingLr, rcFill.top + paddingTb)};
			// const D2D1_POINT_2U pt2{D2D1::Point2U(rcFill.left + paddingLr + contentWidth, pt1.y)};
			// const D2D1_POINT_2U pt3{D2D1::Point2U((pt1.x + pt2.x) / 2, pt1.y + contentTopHeight)};
			// MoveToEx(hdc, pt2.x, pt2.y, nullptr);
			// LineTo(hdc, pt3.x, pt3.y);
			// LineTo(hdc, pt1.x, pt1.y);

			// const LONG contentBottomYPos = static_cast<LONG>(pt3.y + 0.382f * contentHeight);
			// MoveToEx(hdc, pt1.x, contentBottomYPos, nullptr);
			// LineTo(hdc, pt2.x, contentBottomYPos);
			const LONG contentXPos = rcFill.left + paddingLr;
			const LONG contentYPos = rcFill.top + height / 2;
			MoveToEx(hdc, contentXPos, contentYPos, nullptr);
			LineTo(hdc, contentXPos + contentWidth, contentYPos);
			SelectObject(hdc, oldPen);
			DeleteObject(pen);
			ReleaseDC(nativeHandle(), hdc);
		}
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
			else
			{
				m_captionBtnWidth = (GetSystemMetrics(SM_CXSIZE) + 8) * 3 * dpiInfo().physicalToDevice;
			}
		}

		if (!isCompositionEnabled())
		{
			RedrawWindow(nativeHandle(), nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_NOINTERNALPAINT | RDW_ERASENOW);
		}
	}

	bool MainWindow::onClose()
	{
		// if (isPage<DownloadPage>())
		// {
		// 	const DownloadPage& downloadPage = getPage<DownloadPage>();
		// 	if (!downloadPage.isCancelled())
		// 	{
		// 		downloadPage.cancelTask();
		// 		[](MainWindow& self, const DownloadPage& page) -> coro::OnewayTask
		// 		{
		// 			co_await page.joinAsync();
		// 			self.destroyWindow();
		// 		}(*this, downloadPage);
		// 	}
		// 	return true;
		// }
		return false;
	}

	void MainWindow::onBeforeWindowDestroy()
	{
		destroyTray();
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
		// !isCompositionEnabled()
		if (ncBtnHitTest(ptMouse))
		{
			return HTCLIENT;
		}
		return HTNOWHERE;
	}

	void MainWindow::onNcPaint(WParam wParam, LParam lParam)
	{
		// dwm未启用的情况下才会调用这个函数来自绘标题栏新增按钮
		const D2D1_RECT_F rc = rect();
		const float width = rc.right - rc.left;
		constexpr float toTrayBthMarginRight = 0.f;
		const float toTrayBthWidth = m_captionBtnWidth / 3.f;
		const float toTrayBthXPos = width - m_captionBtnWidth - toTrayBthMarginRight - toTrayBthWidth - -m_margins.left;
		m_btnToTray.setBounds(D2D1::Rect(toTrayBthXPos, 6 - m_margins.top, toTrayBthXPos + toTrayBthWidth, -2.f));
		m_btnToTray.setDontDrawDefault(true);
		m_btnToTray.drawImpl(renderContext());
	}

	void MainWindow::onDropFiles(WParam wParam)
	{
		HDROP hDrop = reinterpret_cast<HDROP>(wParam);
		if (UINT length = DragQueryFileW(hDrop, 0, nullptr, 0))
		{
			std::wstring filePath;
			filePath.resize(length + 1);
			length = DragQueryFileW(hDrop, 0, filePath.data(), static_cast<UINT>(filePath.size()));
			if (length && isPage<HomePage>())
			{
				getPage<HomePage>().getLeftSidebar()->getStartAppDiv()->launchFile(filePath);
			}
		}
		DragFinish(hDrop);
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
				HMENU hMenu = CreatePopupMenu();
				if (!hMenu)
				{
					return;
				}
				{
					MENUITEMINFOW menuItem = {sizeof(MENUITEMINFOW)};
					menuItem.fMask = MIIM_ID | MIIM_STRING;
					wchar_t szText[] = {L"退出"};
					menuItem.dwTypeData = szText;
					menuItem.cch = sizeof(szText) / sizeof(wchar_t);
					menuItem.wID = 1;
					InsertMenuItemW(hMenu, 0, TRUE, &menuItem);
				}
				{
					MENUITEMINFOW menuItem = {sizeof(MENUITEMINFOW)};
					menuItem.fMask = MIIM_FTYPE;
					menuItem.fType = MFT_SEPARATOR;
					InsertMenuItemW(hMenu, 0, TRUE, &menuItem);
				}
				std::wstring path;
				if (isPage<HomePage>())
				{
					path = std::wstring{getPage<HomePage>().getLeftSidebar()->getStartAppDiv()->getCurrentSelectedPath()};
				}
				if (!path.empty())
				{
					MENUITEMINFOW menuItem = {sizeof(MENUITEMINFOW)};
					menuItem.fMask = MIIM_ID | MIIM_STRING;
					menuItem.dwTypeData = path.data();
					menuItem.cch = static_cast<UINT>(path.size());
					menuItem.wID = 2;
					InsertMenuItemW(hMenu, 0, TRUE, &menuItem);
				}
				POINT pt{};
				GetCursorPos(&pt);
				SetForegroundWindow(nativeHandle());
				UINT id = TrackPopupMenuEx(hMenu, TPM_LEFTALIGN | TPM_NONOTIFY | TPM_RETURNCMD, pt.x, pt.y, nativeHandle(), nullptr);
				SendMessageW(nativeHandle(), WM_NULL, 0, 0);
				DestroyMenu(hMenu);
				if (id == 1)
				{
					destroyWindow();
				}
				else if (id == 2 && isPage<HomePage>())
				{
					getPage<HomePage>().getLeftSidebar()->getEnvBoxCardArea()->launchProcess(path);
				}
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
			m_btnToTray.setDontDrawDefault(false);
			DWM_SYSTEMBACKDROP_TYPE t = DWMSBT_MAINWINDOW;
			MARGINS extendMargin = m_physicalMargins;
			const HRESULT hr = DwmSetWindowAttribute(nativeHandle(), DWMWA_SYSTEMBACKDROP_TYPE, &t, sizeof(t));
			if (hr != S_OK)
			{
				extendMargin.cxLeftWidth = 0;
				extendMargin.cxRightWidth = 0;
				extendMargin.cyBottomHeight = 0;
				m_margins.left = 0;
				m_margins.right = 0;
				m_margins.bottom = 0;
			}
			DwmExtendFrameIntoClientArea(nativeHandle(), &extendMargin);
		}
		else
		{
			m_btnToTray.setDontDrawDefault(true);
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
		// else if (isPage<DownloadPage>())
		// {
		// 	getPage<DownloadPage>().setMargins(m_margins);
		// 	const D2D_RECT_F rc = rect();
		// 	getPage<HomePage>().onResize(rc.right - rc.left, rc.bottom - rc.top);
		// }
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

	// coro::LazyTask<void> MainWindow::initSymbols()
	// {
	// 	changePageTo<DownloadPage>();
	//
	// 	co_await getPage<DownloadPage>().untilSuccess();
	//
	// 	if (getPage<DownloadPage>().isFileVerified())
	// 	{
	// 		changePageTo<HomePage>();
	// 	}
	// 	co_return;
	// }

	bool MainWindow::ncBtnHitTest(POINT pt) const
	{
		//这里的入参pt是相对于屏幕的
		ScreenToClient(nativeHandle(), &pt);
		return m_btnToTray.hitTest(D2D1::Point2F(pt.x * dpiInfo().physicalToDevice, pt.y * dpiInfo().physicalToDevice));
	}
}
