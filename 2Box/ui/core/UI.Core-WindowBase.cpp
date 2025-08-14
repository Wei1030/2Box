module;
#include "res/resource.h"
module UI.Core;

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
		updateDpi();
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
		destroyWindow();
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
		m_renderCtx.renderTarget.reset();
		m_renderCtx.brush.reset();

		if (m_hWnd)
		{
			DestroyWindow(m_hWnd);
			m_hWnd = nullptr;
		}
	}

	D2D_RECT_F WindowBase::rect() const
	{
		RECT rc;
		GetWindowRect(m_hWnd, &rc);
		const float physicalToDevice = m_dpiInfo.physicalToDevice;
		return D2D1::RectF(rc.left * physicalToDevice, rc.top * physicalToDevice, rc.right * physicalToDevice, rc.bottom * physicalToDevice);
	}

	void WindowBase::setRect(const D2D_RECT_F& rect)
	{
		const float deviceToPhysical = m_dpiInfo.deviceToPhysical;
		SetWindowPos(m_hWnd,
		             nullptr,
		             static_cast<int>(std::ceil(rect.left * deviceToPhysical)),
		             static_cast<int>(std::ceil(rect.top * deviceToPhysical)),
		             static_cast<int>(std::ceil((rect.right - rect.left) * deviceToPhysical)),
		             static_cast<int>(std::ceil((rect.bottom - rect.top) * deviceToPhysical)),
		             SWP_NOZORDER | SWP_NOACTIVATE);
	}

	D2D_RECT_F WindowBase::physicalRect() const
	{
		RECT rc;
		GetWindowRect(m_hWnd, &rc);
		return D2D1::RectF(static_cast<float>(rc.left), static_cast<float>(rc.top), static_cast<float>(rc.right), static_cast<float>(rc.bottom));
	}

	void WindowBase::setPhysicalRect(const D2D_RECT_F& rect)
	{
		SetWindowPos(m_hWnd,
		             nullptr,
		             static_cast<int>(std::ceil(rect.left)),
		             static_cast<int>(std::ceil(rect.top)),
		             static_cast<int>(std::ceil(rect.right - rect.left)),
		             static_cast<int>(std::ceil(rect.bottom - rect.top)),
		             SWP_NOZORDER | SWP_NOACTIVATE);
	}

	void WindowBase::invalidateRect(const D2D_RECT_F& rect)
	{
		const float deviceToPhysical = m_dpiInfo.deviceToPhysical;
		const RECT rc{
			static_cast<int>(std::ceil(rect.left * deviceToPhysical)),
			static_cast<int>(std::ceil(rect.top * deviceToPhysical)),
			static_cast<int>(std::ceil(rect.right * deviceToPhysical)),
			static_cast<int>(std::ceil(rect.bottom * deviceToPhysical))
		};
		InvalidateRect(m_hWnd, &rc, false);
	}

	void WindowBase::invalidateRect()
	{
		InvalidateRect(m_hWnd, nullptr, false);
	}

	D2D_RECT_F WindowBase::rectNeedUpdate() const
	{
		RECT rc;
		GetUpdateRect(m_hWnd, &rc, false);
		const float physicalToDevice = m_dpiInfo.physicalToDevice;
		return D2D1::RectF(rc.left * physicalToDevice, rc.top * physicalToDevice, rc.right * physicalToDevice, rc.bottom * physicalToDevice);
	}

	void WindowBase::reserveRenderers(std::size_t numExcludeControls, std::size_t numControls)
	{
		m_renderersExcludeControls.reserve(numExcludeControls);
		m_controlManager.reserveControls(numControls);
		m_pendingNoDeviceResources.reserve(numExcludeControls + numControls);
	}

	void WindowBase::addRenderer(RendererInterface* renderer)
	{
		m_renderersExcludeControls.push_back(renderer);
		m_pendingNoDeviceResources.push_back(renderer);
	}

	void WindowBase::addControl(ControlBase* control)
	{
		m_controlManager.addControl(control);
		m_pendingNoDeviceResources.push_back(control);
	}

	static void remove_renderer_from_array(std::vector<RendererInterface*>& rendererArray, const RendererInterface* renderer)
	{
		for (auto it = rendererArray.begin(); it != rendererArray.end(); ++it)
		{
			if (*it == renderer)
			{
				rendererArray.erase(it);
				break;
			}
		}
	}

	void WindowBase::removeControl(const ControlBase* control)
	{
		m_controlManager.removeControl(control);
		remove_renderer_from_array(m_pendingNoDeviceResources, control);
	}

	void WindowBase::removeRenderer(const RendererInterface* renderer)
	{
		remove_renderer_from_array(m_renderersExcludeControls, renderer);
		remove_renderer_from_array(m_pendingNoDeviceResources, renderer);
	}

	HResult WindowBase::onRender()
	{
		const RenderContext& ctx = renderContext();
		ctx.renderTarget->BeginDraw();
		ctx.renderTarget->PushAxisAlignedClip(rectNeedUpdate(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		this->draw(ctx);
		ctx.renderTarget->PopAxisAlignedClip();
		return ctx.renderTarget->EndDraw();
	}

	HWND WindowBase::createWindowInternal(DWORD dwExStyle, LPCWSTR lpWindowName, DWORD dwStyle,
	                                      int X, int Y, int nWidth, int nHeight, HWND hWndParent, HMENU hMenu)
	{
		return CreateWindowExW(dwExStyle, MainApp::appName.data(), lpWindowName, dwStyle,
		                       X, Y, nWidth, nHeight, hWndParent, hMenu,
		                       app().moduleInstance(), this);
	}

	HResult WindowBase::prepareDeviceResources()
	{
		auto prepareDeviceResourcesForPending = [this]() -> HResult
		{
			if (!m_pendingNoDeviceResources.empty())
			{
				for (auto it = m_pendingNoDeviceResources.begin(); it != m_pendingNoDeviceResources.end(); ++it)
				{
					if (const HResult hr = (*it)->onCreateDeviceResources(m_renderCtx.renderTarget); FAILED(hr))
					{
						m_pendingNoDeviceResources.clear();
						return hr;
					}
				}
				m_pendingNoDeviceResources.clear();
			}
			return S_OK;
		};

		if (m_renderCtx.renderTarget)
		{
			return prepareDeviceResourcesForPending();
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

		HResult hr = app().d2d1Factory()->CreateHwndRenderTarget(
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
		hr = onCreateDeviceResources(m_renderCtx.renderTarget);
		if (FAILED(hr))
		{
			return hr;
		}
		return prepareDeviceResourcesForPending();
	}

	void WindowBase::releaseDeviceResources()
	{
		m_pendingNoDeviceResources.clear();

		std::vector<ControlBase*>& controls = m_controlManager.getControls();
		for (auto it = controls.begin(); it != controls.end(); ++it)
		{
			(*it)->onDiscardDeviceResources();
			m_pendingNoDeviceResources.push_back(*it);
		}
		for (auto it = m_renderersExcludeControls.begin(); it != m_renderersExcludeControls.end(); ++it)
		{
			(*it)->onDiscardDeviceResources();
			m_pendingNoDeviceResources.push_back(*it);
		}

		m_renderCtx.renderTarget.reset();
		m_renderCtx.brush.reset();
		onDiscardDeviceResources();
	}

	void WindowBase::updateDpi()
	{
		unsigned int dpi;
		if (win32_api::GetDpiForWindow)
		{
			dpi = win32_api::GetDpiForWindow(m_hWnd);
		}
		else
		{
			HDC hdc = GetDC(nullptr);
			dpi = GetDeviceCaps(hdc, LOGPIXELSX);
			ReleaseDC(nullptr, hdc);
		}

		if (dpi)
		{
			m_dpiInfo.dpi = static_cast<float>(dpi);
			m_dpiInfo.physicalToDevice = 96.f / dpi;
			m_dpiInfo.deviceToPhysical = dpi / 96.f;
		}
		else
		{
			m_dpiInfo.dpi = 96.f;
			m_dpiInfo.physicalToDevice = 1.f;
			m_dpiInfo.deviceToPhysical = 1.f;
		}

		if (m_renderCtx.renderTarget)
		{
			m_renderCtx.renderTarget->SetDpi(m_dpiInfo.dpi, m_dpiInfo.dpi);
		}
	}

	void WindowBase::resize(std::uint32_t width, std::uint32_t height)
	{
		if (m_renderCtx.renderTarget)
		{
			// Note: This method can fail, but it's okay to ignore the
			// error here, because the error will be returned again
			// the next time EndDraw is called.
			m_renderCtx.renderTarget->Resize(D2D1::SizeU(width, height));
		}
		onResize(static_cast<float>(width), static_cast<float>(height));
	}

	void WindowBase::mouseMove(int physicalX, int physicalY, MouseEvent::ButtonType button, std::size_t downState)
	{
		const float physicalToDevice = m_dpiInfo.physicalToDevice;
		const MouseEvent e{
			D2D1::Point2F(physicalX * physicalToDevice, physicalY * physicalToDevice),
			button,
			downState
		};
		m_controlManager.onMouseMove(e);
	}

	void WindowBase::mouseDown(int physicalX, int physicalY, MouseEvent::ButtonType button, std::size_t downState)
	{
		const float physicalToDevice = m_dpiInfo.physicalToDevice;
		const MouseEvent e{
			D2D1::Point2F(physicalX * physicalToDevice, physicalY * physicalToDevice),
			button,
			downState
		};
		m_controlManager.onMouseDown(e);
	}

	void WindowBase::mouseUp(int physicalX, int physicalY, MouseEvent::ButtonType button, std::size_t downState)
	{
		const float physicalToDevice = m_dpiInfo.physicalToDevice;
		const MouseEvent e{
			D2D1::Point2F(physicalX * physicalToDevice, physicalY * physicalToDevice),
			button,
			downState
		};
		m_controlManager.onMouseUp(e);
	}

	void WindowBase::onDestroy()
	{
		m_hWnd = nullptr;
		if (m_bIsExitAppWhenWindowDestroyed)
		{
			app().exit();
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
		try
		{
			if (message == WM_CREATE)
			{
				const LPCREATESTRUCTW pcs = reinterpret_cast<LPCREATESTRUCTW>(lParam);
				SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pcs->lpCreateParams));
			}
			else if (message == WM_NCCREATE)
			{
				if (win32_api::EnableNonClientDpiScaling)
				{
					win32_api::EnableNonClientDpiScaling(hWnd);
				}
			}
			else
			{
				if (WindowBase* pWnd = reinterpret_cast<WindowBase*>(GetWindowLongPtrW(hWnd, GWLP_USERDATA)))
				{
					switch (message)
					{
					case WM_DESTROY:
						{
							pWnd->onDestroy();
						}
						return 0;
					case WM_SIZE:
						{
							const std::uint32_t width = LOWORD(lParam);
							const std::uint32_t height = HIWORD(lParam);
							pWnd->resize(width, height);
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
							else
							{
								pWnd->releaseDeviceResources();
							}
							ValidateRect(hWnd, nullptr);
						}
						return 0;
					case WM_CLOSE:
						{
							if (pWnd->onClose())
							{
								return 0;
							}
						}
						break;
					case WM_DISPLAYCHANGE:
						{
							InvalidateRect(hWnd, nullptr, FALSE);
						}
						return 0;
					case WM_MOUSEMOVE:
						pWnd->mouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseEvent::ButtonType::NotInvolved, wParam);
						break;
					case WM_LBUTTONDOWN:
						pWnd->mouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseEvent::ButtonType::Left, wParam);
						break;
					case WM_RBUTTONDOWN:
						pWnd->mouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseEvent::ButtonType::Right, wParam);
						break;
					case WM_MBUTTONDOWN:
						pWnd->mouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseEvent::ButtonType::Middle, wParam);
						break;
					case WM_XBUTTONDOWN:
						{
							const WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
							const WORD fwButton = GET_XBUTTON_WPARAM(wParam);
							const MouseEvent::ButtonType xButton = fwButton == XBUTTON1 ? MouseEvent::ButtonType::X1 : MouseEvent::ButtonType::X2;
							pWnd->mouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), xButton, fwKeys);
						}
						break;
					case WM_LBUTTONUP:
						pWnd->mouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseEvent::ButtonType::Left, wParam);
						break;
					case WM_RBUTTONUP:
						pWnd->mouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseEvent::ButtonType::Right, wParam);
						break;
					case WM_MBUTTONUP:
						pWnd->mouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), MouseEvent::ButtonType::Middle, wParam);
						break;
					case WM_XBUTTONUP:
						{
							const WORD fwKeys = GET_KEYSTATE_WPARAM(wParam);
							const WORD fwButton = GET_XBUTTON_WPARAM(wParam);
							const MouseEvent::ButtonType xButton = fwButton == XBUTTON1 ? MouseEvent::ButtonType::X1 : MouseEvent::ButtonType::X2;
							pWnd->mouseUp(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam), xButton, fwKeys);
						}
						break;
					case WM_DPICHANGED:
						{
							pWnd->updateDpi();
							const RECT* pNewRc = reinterpret_cast<RECT*>(lParam);
							pWnd->setPhysicalRect(D2D1::RectF(static_cast<float>(pNewRc->left), static_cast<float>(pNewRc->top),
							                                  static_cast<float>(pNewRc->right), static_cast<float>(pNewRc->bottom)));
						}
						break;
					default:
						break;
					}
				}
			}
		}
		catch (const std::exception& e)
		{
			app().get_scheduler().addTask([except = std::make_exception_ptr(e)]
			{
				std::rethrow_exception(except);
			});
		}
		catch (...)
		{
			app().get_scheduler().addTask([]
			{
				throw std::runtime_error("Unknown error occur in WndProc!");
			});
		}
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}
}
