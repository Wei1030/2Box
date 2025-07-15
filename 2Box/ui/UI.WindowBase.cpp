module;
#include "res/resource.h"
module UI.WindowBase;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import DynamicWin32Api;

namespace ui
{
	WindowBase::WindowBase(const WindowCreateParam& param)
	{
		WindowBase::registerWndClassOnce();
		m_hWnd = createWindowInternal(param.dwExStyle, param.title.data(), param.dwStyle,
		                              param.x, param.y, param.nWidth, param.nHeight, nullptr, param.hMenu);
		if (nullptr == m_hWnd)
		{
			throw std::runtime_error(std::format("CreateWindowExW fail , error code: {}", GetLastError()));
		}
		if (win32_api::GetDpiForWindow)
		{
			if (const unsigned int dpi = win32_api::GetDpiForWindow(m_hWnd))
			{
				m_physicalToDevice = 96.f / dpi;
				m_deviceToPhysical = dpi / 96.f;
			}
		}
	}

	// WindowBase::WindowBase(WindowBase* parentWnd, const WindowCreateParam& param)
	// {
	// 	if (!parentWnd || !parentWnd->getHandle())
	// 	{
	// 		throw std::runtime_error("parentWnd is null!");
	// 	}
	// 	m_hWnd = createWindowInternal(param.dwExStyle, param.title.data(), param.dwStyle,
	// 								  param.x, param.y, param.nWidth, param.nHeight, parentWnd->getHandle(), param.hMenu);
	// 	if (nullptr == m_hWnd)
	// 	{
	// 		throw std::runtime_error(std::format("CreateWindowExW fail , error code: {}", GetLastError()));
	// 	}
	// 	// TODO: 创建子窗口还需要做管理，如果之后有需要时再实现吧
	// }

	WindowBase::~WindowBase()
	{
		if (m_hWnd)
		{
			DestroyWindow(m_hWnd);
			m_hWnd = nullptr;
		}
	}

	void WindowBase::show(int nCmdShow /*= SW_SHOW*/) const
	{
		if (m_hWnd)
		{
			ShowWindow(m_hWnd, nCmdShow);
			UpdateWindow(m_hWnd);
		}
	}

	void WindowBase::destroyWindow()
	{
		safe_release(&m_renderCtx.renderTarget);
		safe_release(&m_renderCtx.brush);

		if (m_hWnd)
		{
			DestroyWindow(m_hWnd);
			m_hWnd = nullptr;
		}
	}

	D2D_RECT_F WindowBase::rect() const
	{
		RECT rc;
		GetClientRect(m_hWnd, &rc);
		return D2D1::RectF(0, 0, rc.right * m_physicalToDevice, rc.bottom * m_physicalToDevice);
	}

	HRESULT WindowBase::prepareDeviceResources()
	{
		if (m_renderCtx.renderTarget)
		{
			return S_OK;
		}
		if (!m_hWnd)
		{
			return E_FAIL;
		}
		RECT rc;
		GetClientRect(m_hWnd, &rc);

		const D2D1_SIZE_U size = D2D1::SizeU(
			rc.right - rc.left,
			rc.bottom - rc.top
		);
		// Create a Direct2D render target.
		HRESULT hr = app().d2d1Factory()->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(m_hWnd, size),
			&m_renderCtx.renderTarget
		);
		if (FAILED(hr))
		{
			return hr;
		}
		hr = m_renderCtx.renderTarget->CreateSolidColorBrush(D2D1::ColorF{D2D1::ColorF::White}, &m_renderCtx.brush);
		if (FAILED(hr))
		{
			return hr;
		}
		return onCreateDeviceResources();
	}

	void WindowBase::releaseDeviceResources()
	{
		safe_release(&m_renderCtx.renderTarget);
		safe_release(&m_renderCtx.brush);
		onDiscardDeviceResources();
	}

	HWND WindowBase::createWindowInternal(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle,
	                                      int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu)
	{
		return CreateWindowExW(dwExStyle, MainApp::appName.data(), lpWindowName, dwStyle,
		                       X, Y, nWidth, nHeight, hWndParent, hMenu,
		                       app().moduleInstance(), this);
	}

	HRESULT WindowBase::onRender()
	{
		m_renderCtx.renderTarget->BeginDraw();
		m_renderCtx.renderTarget->SetTransform(D2D1::Matrix3x2F::Identity());
		m_renderCtx.renderTarget->Clear(D2D1::ColorF(D2D1::ColorF::White));

		return m_renderCtx.renderTarget->EndDraw();
	}

	void WindowBase::resize(std::uint32_t width, std::uint32_t height) const
	{
		if (m_renderCtx.renderTarget)
		{
			// Note: This method can fail, but it's okay to ignore the
			// error here, because the error will be returned again
			// the next time EndDraw is called.
			m_renderCtx.renderTarget->Resize(D2D1::SizeU(width, height));
		}
	}

	void WindowBase::onDestroy()
	{
		m_hWnd = nullptr;
		if (m_bIsExitAppWhenWindowDestroyed)
		{
			PostQuitMessage(0);
		}
	}

	void WindowBase::registerWndClassOnce()
	{
		struct RegisterWndClsHelper
		{
			RegisterWndClsHelper()
			{
				WNDCLASSEXW wndClsEx = {sizeof(WNDCLASSEXW)};
				wndClsEx.style = CS_HREDRAW | CS_VREDRAW;
				wndClsEx.lpfnWndProc = &WindowBase::wndProc;
				wndClsEx.cbClsExtra = 0;
				wndClsEx.cbWndExtra = sizeof(std::uintptr_t);
				wndClsEx.hInstance = app().moduleInstance();
				wndClsEx.hIcon = LoadIcon(app().moduleInstance(), MAKEINTRESOURCE(IDI_APP_ICON));
				wndClsEx.hbrBackground = nullptr;
				wndClsEx.lpszMenuName = nullptr;
				wndClsEx.hCursor = LoadCursor(nullptr, IDI_APPLICATION);
				wndClsEx.lpszClassName = MainApp::appName.data();

				if (0 == RegisterClassExW(&wndClsEx))
				{
					throw std::runtime_error(std::format("RegisterClassExW fail, error code: {}", GetLastError()));
				}
			}
		};
		[[maybe_unused]] static RegisterWndClsHelper initOnce{};
	}

	LRESULT WindowBase::wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
	{
		if (message == WM_CREATE)
		{
			const LPCREATESTRUCTW pcs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
			SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pcs->lpCreateParams));
		}
		else
		{
			if (WindowBase* pWnd = reinterpret_cast<WindowBase*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)))
			{
				switch (message)
				{
				case WM_SIZE:
					{
						const std::uint32_t width = LOWORD(lParam);
						const std::uint32_t height = HIWORD(lParam);
						pWnd->resize(width, height);
						pWnd->onResize(width, height);
					}
					return 0;
				case WM_DISPLAYCHANGE:
					{
						InvalidateRect(hWnd, nullptr, FALSE);
					}
					return 0;
				case WM_PAINT:
					{
						if (SUCCEEDED(pWnd->prepareDeviceResources()))
						{
							if (FAILED(pWnd->onRender()))
							{
								pWnd->releaseDeviceResources();
							}
						}
						ValidateRect(hWnd, nullptr);
					}
					return 0;

				case WM_DESTROY:
					{
						pWnd->onDestroy();
					}
					return 0;
				default:
					break;
				}
			}
		}

		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
}
