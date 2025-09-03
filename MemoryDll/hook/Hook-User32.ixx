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
			c.addToplevelWindow(hWnd, global::Data::get().envFlag());
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
			c.removeToplevelWindow(hWnd, global::Data::get().envFlag());
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

	std::vector<HWND> get_all_toplevel_windows()
	{
		std::vector<HWND> result;
		try
		{
			const rpc::ClientDefault c;
			std::uint64_t wnds[rpc::MAX_TOPLEVEL_WND_COUNT]{};
			std::uint32_t count = rpc::MAX_TOPLEVEL_WND_COUNT;
			c.getAllToplevelWindow(global::Data::get().envFlag(), wnds, &count);
			result.reserve(count);
			for (std::uint32_t i = 0; i < count; ++i)
			{
				result.push_back(reinterpret_cast<HWND>(wnds[i]));
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

	template <auto Trampoline>
	HWND WINAPI FindWindowA(_In_opt_ LPCSTR lpClassName, _In_opt_ LPCSTR lpWindowName)
	{
		HWND hWnd = Trampoline(lpClassName, lpWindowName);
		if (hWnd && contains_toplevel_window(hWnd))
		{
			return hWnd;
		}
		return nullptr;
	}

	template <auto Trampoline>
	HWND WINAPI FindWindowW(_In_opt_ LPCWSTR lpClassName, _In_opt_ LPCWSTR lpWindowName)
	{
		HWND hWnd = Trampoline(lpClassName, lpWindowName);
		if (hWnd && contains_toplevel_window(hWnd))
		{
			return hWnd;
		}
		return nullptr;
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
		if (hWnd && contains_toplevel_window(hWnd))
		{
			return hWnd;
		}
		return nullptr;
	}

	template <auto Trampoline>
	HWND WINAPI FindWindowExW(_In_opt_ HWND hWndParent, _In_opt_ HWND hWndChildAfter, _In_opt_ LPCWSTR lpszClass, _In_opt_ LPCWSTR lpszWindow)
	{
		if (hWndParent && hWndParent != HWND_MESSAGE && hWndParent != get_desktop_window())
		{
			return Trampoline(hWndParent, hWndChildAfter, lpszClass, lpszWindow);
		}

		HWND hWnd = Trampoline(hWndParent, hWndChildAfter, lpszClass, lpszWindow);
		if (hWnd && contains_toplevel_window(hWnd))
		{
			return hWnd;
		}
		return nullptr;
	}

	struct MyEnumWndParams
	{
		WNDENUMPROC lpEnumFunc;
		LPARAM lParam;
	};

	BOOL CALLBACK my_enum_windows_proc(HWND hWnd, LPARAM lParam)
	{
		const MyEnumWndParams* myParam = reinterpret_cast<MyEnumWndParams*>(lParam);
		if (!hWnd || contains_toplevel_window(hWnd))
		{
			return myParam->lpEnumFunc(hWnd, myParam->lParam);
		}
		return TRUE;
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

	BOOL CALLBACK enum_wnd_helper(HWND hWnd, LPARAM lParam)
	{
		std::vector<HWND>& allWnd = *reinterpret_cast<std::vector<HWND>*>(lParam);
		allWnd.push_back(hWnd);
		return TRUE;
	}

	std::vector<HWND> get_all_toplevel_windows_by_enum()
	{
		std::vector<HWND> allToplevelWindows;
		allToplevelWindows.reserve(32);
		::EnumWindows(&enum_wnd_helper, reinterpret_cast<LPARAM>(&allToplevelWindows));
		return allToplevelWindows;
	}

	BOOL CALLBACK get_first_toplevel_wnd_helper(HWND hWnd, LPARAM lParam)
	{
		HWND* hWndResult = reinterpret_cast<HWND*>(lParam);
		*hWndResult = hWnd;
		return FALSE;
	}

	HWND get_first_toplevel_window()
	{
		HWND hWnd{nullptr};
		::EnumWindows(&get_first_toplevel_wnd_helper, reinterpret_cast<LPARAM>(&hWnd));
		return hWnd;
	}

	HWND get_next_toplevel_window(HWND hWnd, UINT uCmd)
	{
		if (uCmd == GW_HWNDFIRST)
		{
			return get_first_toplevel_window();
		}

		std::vector<HWND> allToplevelWindows = get_all_toplevel_windows_by_enum();
		if (allToplevelWindows.empty())
		{
			return nullptr;
		}
		if (uCmd == GW_HWNDLAST)
		{
			return allToplevelWindows.back();
		}

		for (size_t i = 0; i < allToplevelWindows.size(); ++i)
		{
			if (allToplevelWindows[i] == hWnd)
			{
				if (uCmd == GW_HWNDPREV)
				{
					if (i == 0)
					{
						return nullptr;
					}
					return allToplevelWindows[i - 1];
				}
				if (uCmd == GW_HWNDNEXT)
				{
					if (i == allToplevelWindows.size() - 1)
					{
						return nullptr;
					}
					return allToplevelWindows[i + 1];
				}
			}
		}
		return allToplevelWindows.front();
	}

	template <auto Trampoline>
	HWND WINAPI GetWindow(_In_ HWND hWnd, _In_ UINT uCmd)
	{
		if (uCmd == GW_ENABLEDPOPUP || uCmd == GW_OWNER)
		{
			HWND hResult = Trampoline(hWnd, uCmd);
			if (GetWindowStyle(hResult) & WS_CHILD)
			{
				return hResult;
			}
			if (contains_toplevel_window(hResult))
			{
				return hResult;
			}
			return nullptr;
		}

		if (uCmd == GW_CHILD)
		{
			if (hWnd == get_desktop_window())
			{
				return get_first_toplevel_window();
			}
			return Trampoline(hWnd, uCmd);
		}

		if (GetWindowStyle(hWnd) & WS_CHILD)
		{
			return Trampoline(hWnd, uCmd);
		}
		return get_next_toplevel_window(hWnd, uCmd);
	}

	template <auto Trampoline>
	HWND WINAPI GetTopWindow(_In_opt_ HWND hWnd)
	{
		if (hWnd && hWnd != get_desktop_window())
		{
			return Trampoline(hWnd);
		}
		return get_first_toplevel_window();
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
