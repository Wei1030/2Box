export module Hook:User32;

import "sys_defs.h";
import :Core;
import GlobalData;
import RpcClient;

namespace hook
{
	void add_toplevel_window(void* hWnd)
	{
		try
		{
			const rpc::ClientDefault c;
			c.addToplevelWindow(hWnd, GetCurrentProcessId(), global::Data::get().envFlag());
		}
		catch (...)
		{
		}
	}

	void remove_toplevel_window(void* hWnd)
	{
		try
		{
			const rpc::ClientDefault c;
			c.removeToplevelWindow(hWnd, GetCurrentProcessId(), global::Data::get().envFlag());
		}
		catch (...)
		{
		}
	}

	bool contains_toplevel_window_in_other_env(void* hWnd)
	{
		try
		{
			const rpc::ClientDefault c;
			return c.containsToplevelWindowExclude(hWnd, global::Data::get().envFlag());
		}
		catch (...)
		{
		}
		return true;
	}

	std::vector<HWND> get_all_toplevel_window_in_other_env()
	{
		std::vector<HWND> result;
		try
		{
			const rpc::ClientDefault c;
			std::uint64_t hWnds[rpc::MAX_TOPLEVEL_WND_COUNT]{};
			std::uint32_t count = rpc::MAX_TOPLEVEL_WND_COUNT;
			c.getAllToplevelWindowExclude(global::Data::get().envFlag(), hWnds, &count);
			result.reserve(count);
			for (std::uint32_t i = 0; i < count; ++i)
			{
				HWND hWnd = reinterpret_cast<HWND>(hWnds[i]);
				if (GetWindowExStyle(hWnd) & WS_EX_TOOLWINDOW)
				{
					continue;
				}
				if (const HWND owner = GetWindow(hWnd, GW_OWNER); owner && !IsWindowVisible(owner))
				{
					continue;
				}
				result.push_back(hWnd);
			}
		}
		catch (...)
		{
		}
		return result;
	}

	template <auto Trampoline>
	HWND WINAPI CreateWindowExA(_In_ DWORD dwExStyle, _In_opt_ LPCSTR lpClassName, _In_opt_ LPCSTR lpWindowName,
	                            _In_ DWORD dwStyle, _In_ int X, _In_ int Y, _In_ int nWidth, _In_ int nHeight,
	                            _In_opt_ HWND hWndParent, _In_opt_ HMENU hMenu, _In_opt_ HINSTANCE hInstance, _In_opt_ LPVOID lpParam)
	{
		HWND hWnd = Trampoline(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		if (!hWnd)
		{
			return hWnd;
		}
		if (dwStyle & WS_CHILD)
		{
			return hWnd;
		}

		add_toplevel_window(hWnd);
		return hWnd;
	}

	template <auto Trampoline>
	HWND WINAPI CreateWindowExW(_In_ DWORD dwExStyle, _In_opt_ LPCWSTR lpClassName, _In_opt_ LPCWSTR lpWindowName,
	                            _In_ DWORD dwStyle, _In_ int X, _In_ int Y, _In_ int nWidth, _In_ int nHeight,
	                            _In_opt_ HWND hWndParent, _In_opt_ HMENU hMenu, _In_opt_ HINSTANCE hInstance, _In_opt_ LPVOID lpParam)
	{
		HWND hWnd = Trampoline(dwExStyle, lpClassName, lpWindowName, dwStyle, X, Y, nWidth, nHeight, hWndParent, hMenu, hInstance, lpParam);
		if (!hWnd)
		{
			return hWnd;
		}
		if (dwStyle & WS_CHILD)
		{
			return hWnd;
		}

		add_toplevel_window(hWnd);
		return hWnd;
	}

	template <auto Trampoline>
	BOOL WINAPI DestroyWindow(_In_ HWND hWnd)
	{
		const bool bIsChildStyle = GetWindowStyle(hWnd) & WS_CHILD;
		if (Trampoline(hWnd))
		{
			if (!bIsChildStyle)
			{
				remove_toplevel_window(hWnd);
			}
			return TRUE;
		}
		return FALSE;
	}

	class SyncInput
	{
	public:
		static SyncInput& instPerThread()
		{
			thread_local SyncInput syncInput;
			return syncInput;
		}

		void startSync(bool bStart)
		{
			m_bLeaderStart = false;
			m_bSyncStart = bStart;
		}

		bool isSync() const { return m_bSyncStart; }

		void startLeader(bool bStart)
		{
			m_bSyncStart = false;
			m_bLeaderStart = bStart;
			if (m_bLeaderStart)
			{
				m_others = get_all_toplevel_window_in_other_env();
			}
			postMsg(global::Data::get().inputSyncMsgId(), static_cast<WPARAM>(m_bLeaderStart), 0, true);
			MessageBeep(MB_OK);
		}

		bool isLeader() const { return m_bLeaderStart; }

		void setLastInput(HWND hWnd, int x, int y)
		{
			m_ptLastPos.x = x;
			m_ptLastPos.y = y;
			m_wndLastProcessMouseMsg = hWnd;
		}

		static bool filterWnd(HWND hWnd)
		{
			return IsWindowVisible(hWnd) && IsWindowEnabled(hWnd) && !IsMinimized(hWnd);
		}

		void postMsg(UINT uMsg, WPARAM wParam, LPARAM lParam, bool bForceAll = false) const
		{
			for (const HWND& hWnd : m_others)
			{
				if (bForceAll || filterWnd(hWnd))
				{
					PostMessageW(hWnd, uMsg, wParam, lParam);
				}
			}
		}

		void postCommonMouseMsg(HWND self, int x, int y, UINT uMsg, WPARAM wParam) const
		{
			RECT selfRc;
			GetClientRect(self, &selfRc);
			const LONG width = selfRc.right - selfRc.left;
			const LONG height = selfRc.bottom - selfRc.top;
			const float wPercent = static_cast<float>(x) / static_cast<float>(width);
			const float hPercent = static_cast<float>(y) / static_cast<float>(height);

			for (const HWND& hWnd : m_others)
			{
				if (filterWnd(hWnd))
				{
					RECT rc;
					GetClientRect(hWnd, &rc);
					const short xPos = static_cast<short>((rc.right - rc.left) * wPercent);
					const short yPos = static_cast<short>((rc.bottom - rc.top) * hPercent);
					PostMessageW(hWnd, uMsg, wParam, MAKELPARAM(xPos, yPos));
				}
			}
		}

		HWND getLastProcessMouseMsgWnd() const { return m_wndLastProcessMouseMsg; }
		POINT getLastPoint() const { return m_ptLastPos; }

	private:
		bool m_bSyncStart{false};
		bool m_bLeaderStart{false};
		POINT m_ptLastPos{};
		HWND m_wndLastProcessMouseMsg{nullptr};
		std::vector<HWND> m_others;
	};

	void process_msg(_In_ CONST MSG* lpMsg)
	{
		if (lpMsg->message == global::Data::get().inputSyncMsgId())
		{
			SyncInput::instPerThread().startSync(lpMsg->wParam ? true : false);
			if (lpMsg->wParam)
			{
				PostMessageW(lpMsg->hwnd, WM_SETFOCUS, 0, 0);
			}
		}
		else if (lpMsg->message >= WM_KEYFIRST && lpMsg->message <= WM_KEYLAST)
		{
			auto processKeyMsg = [&]()
			{
				if (lpMsg->message != WM_KEYDOWN)
				{
					return false;
				}
				if (lpMsg->wParam != 0x53 // S
					&& lpMsg->wParam != 0x43) // C
				{
					return false;
				}
				if (lpMsg->lParam & 0x40000000) // autorepeat
				{
					return false;
				}
				if ((GetKeyState(VK_CONTROL) & 0x8000) == 0
					|| (GetKeyState(VK_MENU) & 0x8000) == 0)
				{
					return false;
				}
				if (lpMsg->wParam == 0x53)
				{
					SyncInput::instPerThread().startLeader(true);
				}
				else if (lpMsg->wParam == 0x43)
				{
					SyncInput::instPerThread().startLeader(false);
				}
				return true;
			};
			if (!processKeyMsg())
			{
				if (SyncInput::instPerThread().isLeader())
				{
					SyncInput::instPerThread().postMsg(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				}
			}
		}
		else if (lpMsg->message == WM_MOUSELEAVE || lpMsg->message == WM_KILLFOCUS)
		{
			if (SyncInput::instPerThread().isSync())
			{
				const_cast<MSG*>(lpMsg)->message = WM_NULL;
			}
		}
		else if (lpMsg->message == WM_MOUSEHOVER
			|| (lpMsg->message >= WM_MOUSEFIRST && lpMsg->message <= WM_MOUSELAST))
		{
			if (lpMsg->message != WM_MOUSEWHEEL && lpMsg->message != WM_MOUSEHWHEEL)
			{
				const int x = GET_X_LPARAM(lpMsg->lParam);
				const int y = GET_Y_LPARAM(lpMsg->lParam);
				SyncInput::instPerThread().setLastInput(lpMsg->hwnd, x, y);
				if (SyncInput::instPerThread().isLeader())
				{
					SyncInput::instPerThread().postCommonMouseMsg(lpMsg->hwnd, x, y, lpMsg->message, lpMsg->wParam);
				}
			}
			else
			{
				if (SyncInput::instPerThread().isLeader())
				{
					SyncInput::instPerThread().postMsg(lpMsg->message, lpMsg->wParam, lpMsg->lParam);
				}
			}
		}
	}

	template <auto Trampoline>
	LRESULT WINAPI DispatchMessageA(_In_ CONST MSG* lpMsg)
	{
		if (lpMsg)
		{
			process_msg(lpMsg);
		}
		return Trampoline(lpMsg);
	}

	template <auto Trampoline>
	LRESULT WINAPI DispatchMessageW(_In_ CONST MSG* lpMsg)
	{
		if (lpMsg)
		{
			process_msg(lpMsg);
		}
		return Trampoline(lpMsg);
	}

	template <auto Trampoline>
	BOOL WINAPI GetCursorPos(_Out_ LPPOINT lpPoint)
	{
		if (lpPoint && SyncInput::instPerThread().isSync())
		{
			POINT clientOffset{};
			ClientToScreen(SyncInput::instPerThread().getLastProcessMouseMsgWnd(), &clientOffset);
			const POINT lastPt = SyncInput::instPerThread().getLastPoint();
			lpPoint->x = lastPt.x + clientOffset.x;
			lpPoint->y = lastPt.y + clientOffset.y;
			return true;
		}
		return Trampoline(lpPoint);
	}

	template <auto Trampoline>
	BOOL WINAPI GetCursorInfo(_Inout_ PCURSORINFO pci)
	{
		const BOOL bRet = Trampoline(pci);
		if (bRet && pci && SyncInput::instPerThread().isSync())
		{
			POINT clientOffset{};
			ClientToScreen(SyncInput::instPerThread().getLastProcessMouseMsgWnd(), &clientOffset);
			const POINT lastPt = SyncInput::instPerThread().getLastPoint();
			pci->ptScreenPos.x = lastPt.x + clientOffset.x;
			pci->ptScreenPos.y = lastPt.y + clientOffset.y;
		}
		return bRet;
	}

	template <auto Trampoline>
	BOOL WINAPI SetCursorPos(_In_ int X, _In_ int Y)
	{
		if (SyncInput::instPerThread().isSync())
		{
			return TRUE;
		}
		return Trampoline(X, Y);
	}

	template <auto Trampoline>
	BOOL WINAPI ClipCursor(_In_opt_ CONST RECT* lpRect)
	{
		if (SyncInput::instPerThread().isSync())
		{
			return TRUE;
		}
		return Trampoline(lpRect);
	}

	template <auto Trampoline>
	HWND WINAPI GetForegroundWindow()
	{
		if (SyncInput::instPerThread().isSync())
		{
			return SyncInput::instPerThread().getLastProcessMouseMsgWnd();
		}
		return Trampoline();
	}

	template <auto Trampoline>
	HWND WINAPI FindWindowA(_In_opt_ LPCSTR lpClassName, _In_opt_ LPCSTR lpWindowName)
	{
		HWND hWnd = Trampoline(lpClassName, lpWindowName);
		if (!hWnd || contains_toplevel_window_in_other_env(hWnd))
		{
			return nullptr;
		}
		return hWnd;
	}

	template <auto Trampoline>
	HWND WINAPI FindWindowW(_In_opt_ LPCWSTR lpClassName, _In_opt_ LPCWSTR lpWindowName)
	{
		HWND hWnd = Trampoline(lpClassName, lpWindowName);
		if (!hWnd || contains_toplevel_window_in_other_env(hWnd))
		{
			return nullptr;
		}
		return hWnd;
	}

	HWND get_desktop_window()
	{
		static HWND hDesktopWindow = GetDesktopWindow();
		return hDesktopWindow;
	}

	template <auto Trampoline>
	HWND WINAPI FindWindowExA(_In_opt_ HWND hWndParent, _In_opt_ HWND hWndChildAfter, _In_opt_ LPCSTR lpszClass, _In_opt_ LPCSTR lpszWindow)
	{
		if (hWndParent && hWndParent != HWND_MESSAGE && hWndParent != get_desktop_window())
		{
			return Trampoline(hWndParent, hWndChildAfter, lpszClass, lpszWindow);
		}

		HWND hWnd = Trampoline(hWndParent, hWndChildAfter, lpszClass, lpszWindow);
		if (!hWnd || contains_toplevel_window_in_other_env(hWnd))
		{
			return nullptr;
		}
		return hWnd;
	}

	template <auto Trampoline>
	HWND WINAPI FindWindowExW(_In_opt_ HWND hWndParent, _In_opt_ HWND hWndChildAfter, _In_opt_ LPCWSTR lpszClass, _In_opt_ LPCWSTR lpszWindow)
	{
		if (hWndParent && hWndParent != HWND_MESSAGE && hWndParent != get_desktop_window())
		{
			return Trampoline(hWndParent, hWndChildAfter, lpszClass, lpszWindow);
		}

		HWND hWnd = Trampoline(hWndParent, hWndChildAfter, lpszClass, lpszWindow);
		if (!hWnd || contains_toplevel_window_in_other_env(hWnd))
		{
			return nullptr;
		}
		return hWnd;
	}

	struct MyEnumWndParams
	{
		WNDENUMPROC lpEnumFunc;
		LPARAM lParam;
	};

	BOOL CALLBACK my_enum_windows_proc(HWND hWnd, LPARAM lParam)
	{
		const MyEnumWndParams* myParam = reinterpret_cast<MyEnumWndParams*>(lParam);
		if (hWnd && contains_toplevel_window_in_other_env(hWnd))
		{
			return TRUE;
		}
		return myParam->lpEnumFunc(hWnd, myParam->lParam);
	}

	template <auto Trampoline>
	BOOL WINAPI EnumWindows(_In_ WNDENUMPROC lpEnumFunc, _In_ LPARAM lParam)
	{
		MyEnumWndParams myParam;
		myParam.lpEnumFunc = lpEnumFunc;
		myParam.lParam = lParam;
		return Trampoline(&my_enum_windows_proc, reinterpret_cast<LPARAM>(&myParam));
	}

	template <auto Trampoline>
	BOOL WINAPI EnumChildWindows(_In_opt_ HWND hWndParent, _In_ WNDENUMPROC lpEnumFunc, _In_ LPARAM lParam)
	{
		if (hWndParent && hWndParent != get_desktop_window())
		{
			return Trampoline(hWndParent, lpEnumFunc, lParam);
		}
		MyEnumWndParams myParam;
		myParam.lpEnumFunc = lpEnumFunc;
		myParam.lParam = lParam;
		return Trampoline(hWndParent, &my_enum_windows_proc, reinterpret_cast<LPARAM>(&myParam));
	}

	template <auto Trampoline>
	BOOL WINAPI EnumDesktopWindows(_In_opt_ HDESK hDesktop, _In_ WNDENUMPROC lpfn, _In_ LPARAM lParam)
	{
		MyEnumWndParams myParam;
		myParam.lpEnumFunc = lpfn;
		myParam.lParam = lParam;
		return Trampoline(hDesktop, &my_enum_windows_proc, reinterpret_cast<LPARAM>(&myParam));
	}

	template <auto Trampoline>
	HWND ensure_not_in_other_env(HWND hWnd, UINT uCmd)
	{
		if (!contains_toplevel_window_in_other_env(hWnd))
		{
			return hWnd;
		}

		if (uCmd != GW_HWNDNEXT || uCmd != GW_HWNDPREV)
		{
			return nullptr;
		}

		HWND hResult = Trampoline(hWnd, uCmd);
		int tryCount = 1000;
		while (hResult && tryCount > 0 && contains_toplevel_window_in_other_env(hResult))
		{
			hResult = Trampoline(hWnd, uCmd);
			tryCount--;
		}
		return hResult;
	}

	template <auto Trampoline>
	HWND WINAPI GetWindow(_In_ HWND hWnd, _In_ UINT uCmd)
	{
		if (!hWnd)
		{
			return Trampoline(hWnd, uCmd);
		}

		if (uCmd == GW_ENABLEDPOPUP || uCmd == GW_OWNER)
		{
			HWND hResult = Trampoline(hWnd, uCmd);
			if (GetWindowStyle(hResult) & WS_CHILD)
			{
				return hResult;
			}
			if (!contains_toplevel_window_in_other_env(hResult))
			{
				return hResult;
			}
			return nullptr;
		}

		if (uCmd == GW_CHILD)
		{
			if (hWnd == get_desktop_window())
			{
				return ensure_not_in_other_env<Trampoline>(Trampoline(hWnd, uCmd), GW_HWNDNEXT);
			}
			return Trampoline(hWnd, uCmd);
		}

		if (GetWindowStyle(hWnd) & WS_CHILD)
		{
			return Trampoline(hWnd, uCmd);
		}

		if (uCmd == GW_HWNDFIRST)
		{
			return ensure_not_in_other_env<Trampoline>(Trampoline(hWnd, uCmd), GW_HWNDNEXT);
		}
		if (uCmd == GW_HWNDLAST)
		{
			return ensure_not_in_other_env<Trampoline>(Trampoline(hWnd, uCmd), GW_HWNDPREV);
		}
		return ensure_not_in_other_env<Trampoline>(Trampoline(hWnd, uCmd), uCmd);
	}

	template <auto Trampoline>
	HWND WINAPI GetTopWindow(_In_opt_ HWND hWnd)
	{
		if (hWnd && hWnd != get_desktop_window())
		{
			return Trampoline(hWnd);
		}
		return ::GetWindow(get_desktop_window(), GW_CHILD);
	}

	void hook_user32()
	{
		// 顶层窗口管理
		create_hook_by_func_ptr<&::CreateWindowExA>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&CreateWindowExA<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::CreateWindowExW>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&CreateWindowExW<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::DestroyWindow>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&DestroyWindow<trampolineConst.value>};
		});

		// 同步输入
		create_hook_by_func_ptr<&::DispatchMessageA>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&DispatchMessageA<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::DispatchMessageW>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&DispatchMessageW<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::GetCursorPos>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&GetCursorPos<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::GetCursorInfo>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&GetCursorInfo<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::SetCursorPos>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&SetCursorPos<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::ClipCursor>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&ClipCursor<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::GetForegroundWindow>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&GetForegroundWindow<trampolineConst.value>};
		});

		// 限制窗口访问
		create_hook_by_func_ptr<&::FindWindowA>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&FindWindowA<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::FindWindowW>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&FindWindowW<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::FindWindowExA>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&FindWindowExA<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::FindWindowExW>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&FindWindowExW<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::EnumWindows>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&EnumWindows<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::EnumChildWindows>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&EnumChildWindows<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::EnumDesktopWindows>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&EnumDesktopWindows<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::GetWindow>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&GetWindow<trampolineConst.value>};
		});
		create_hook_by_func_ptr<&::GetTopWindow>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&GetTopWindow<trampolineConst.value>};
		});
	}
}
