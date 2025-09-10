export module Hook:Shell32;

import "sys_defs.h";
import std;
import :Core;
import GlobalData;
import RpcClient;

namespace hook
{
	template <auto Trampoline>
	HINSTANCE STDAPICALLTYPE ShellExecuteA(_In_opt_ HWND hwnd, _In_opt_ LPCSTR lpOperation, _In_ LPCSTR lpFile, _In_opt_ LPCSTR lpParameters, _In_opt_ LPCSTR lpDirectory, _In_ INT nShowCmd)
	{
		if (lpOperation)
		{
			if (_stricmp(lpOperation, "runas") == 0)
			{
				const std::filesystem::path path{lpFile ? lpFile : ""};
				rpc::default_call_ignore_error(&rpc::ClientDefault::requireElevation, GetCurrentProcessId(), global::Data::get().envFlag(), path.native());
				return reinterpret_cast<HINSTANCE>(SE_ERR_ACCESSDENIED);
			}
		}
		return Trampoline(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	}

	template <auto Trampoline>
	HINSTANCE STDAPICALLTYPE ShellExecuteW(_In_opt_ HWND hwnd, _In_opt_ LPCWSTR lpOperation, _In_ LPCWSTR lpFile, _In_opt_ LPCWSTR lpParameters, _In_opt_ LPCWSTR lpDirectory, _In_ INT nShowCmd)
	{
		if (lpOperation)
		{
			if (_wcsicmp(lpOperation, L"runas") == 0)
			{
				rpc::default_call_ignore_error(&rpc::ClientDefault::requireElevation, GetCurrentProcessId(), global::Data::get().envFlag(), lpFile ? lpFile : L"");
				return reinterpret_cast<HINSTANCE>(SE_ERR_ACCESSDENIED);
			}
		}
		return Trampoline(hwnd, lpOperation, lpFile, lpParameters, lpDirectory, nShowCmd);
	}

	template <auto Trampoline>
	BOOL STDAPICALLTYPE ShellExecuteExA(_Inout_ SHELLEXECUTEINFOA* pExecInfo)
	{
		if (pExecInfo && pExecInfo->lpVerb)
		{
			if (_stricmp(pExecInfo->lpVerb, "runas") == 0)
			{
				pExecInfo->hInstApp = reinterpret_cast<HINSTANCE>(SE_ERR_ACCESSDENIED);
				SetLastError(ERROR_CANCELLED);
				const std::filesystem::path path{pExecInfo->lpFile ? pExecInfo->lpFile : ""};
				rpc::default_call_ignore_error(&rpc::ClientDefault::requireElevation, GetCurrentProcessId(), global::Data::get().envFlag(), path.native());
				return FALSE;
			}
		}
		return Trampoline(pExecInfo);
	}

	template <auto Trampoline>
	BOOL STDAPICALLTYPE ShellExecuteExW(_Inout_ SHELLEXECUTEINFOW* pExecInfo)
	{
		if (pExecInfo && pExecInfo->lpVerb)
		{
			if (_wcsicmp(pExecInfo->lpVerb, L"runas") == 0)
			{
				pExecInfo->hInstApp = reinterpret_cast<HINSTANCE>(SE_ERR_ACCESSDENIED);
				SetLastError(ERROR_CANCELLED);
				rpc::default_call_ignore_error(&rpc::ClientDefault::requireElevation, GetCurrentProcessId(), global::Data::get().envFlag(), pExecInfo->lpFile ? pExecInfo->lpFile : L"");
				return FALSE;
			}
		}
		return Trampoline(pExecInfo);
	}

	void hook_shell32()
	{
		// 如果当前进程是未提升的，就需要hook ShellExecuteExW禁止创建管理员进程
		// 但是 通过cmd运行的程序，不是通过runas谓词来创建管理员程序的。不想过于深入研究，这种情况换个思路
		// cmd首先会尝试正常创建进程，如果产生了ERROR_ELEVATION_REQUIRED错误，才会尝试以管理员身份运行。
		// 所以这种情况，在CreateProcessW中我们把ERROR_ELEVATION_REQUIRED错误抹去换个错误就好了。
		if (!global::Data::get().isNonLimitedAdmin())
		{
			create_hook_by_func_ptr<&::ShellExecuteA>().setHookFromGetter([&](auto trampolineConst)
			{
				return HookInfo{&ShellExecuteA<trampolineConst.value>};
			});
			create_hook_by_func_ptr<&::ShellExecuteW>().setHookFromGetter([&](auto trampolineConst)
			{
				return HookInfo{&ShellExecuteW<trampolineConst.value>};
			});
			create_hook_by_func_ptr<&::ShellExecuteExA>().setHookFromGetter([&](auto trampolineConst)
			{
				return HookInfo{&ShellExecuteExA<trampolineConst.value>};
			});
			create_hook_by_func_ptr<&::ShellExecuteExW>().setHookFromGetter([&](auto trampolineConst)
			{
				return HookInfo{&ShellExecuteExW<trampolineConst.value>};
			});
		}
	}
}
