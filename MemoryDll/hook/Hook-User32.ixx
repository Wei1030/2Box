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

	bool contains_toplevel_window(void* hWnd)
	{
		try
		{
			const rpc::ClientDefault c;
			return c.containsToplevelWindow(hWnd, global::Data::get().envFlag());
		}
		catch (...)
		{
		}
		return false;
	}

	bool contains_toplevel_window_in_other_env(void* hWnd)
	{
		try
		{
			const rpc::ClientDefault c;
			return c.containsToplevelWindowExcludingByFlag(hWnd, global::Data::get().envFlag());
		}
		catch (...)
		{
		}
		return true;
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
