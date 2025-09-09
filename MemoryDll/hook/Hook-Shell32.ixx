export module Hook:Shell32;

import "sys_defs.h";
import std;
import :Core;
import GlobalData;

namespace hook
{
	template <auto Trampoline>
	BOOL STDAPICALLTYPE ShellExecuteExW(_Inout_ SHELLEXECUTEINFOW* pExecInfo)
	{
		if (pExecInfo && pExecInfo->lpVerb)
		{
			std::wstring lowerVerb(pExecInfo->lpVerb);
			// ReSharper disable once CppUseRangeAlgorithm
			std::transform(lowerVerb.begin(), lowerVerb.end(), lowerVerb.begin(), std::towlower);
			if (lowerVerb == L"runas")
			{
				pExecInfo->hInstApp = reinterpret_cast<HINSTANCE>(SE_ERR_ACCESSDENIED);
				SetLastError(ERROR_CANCELLED);
				return FALSE;
			}
		}
		return Trampoline(pExecInfo);
	}

	void hook_shell32()
	{
		// 如何当前进程是未提升的，就需要hook ShellExecuteExW禁止创建管理员进程
		if (!global::Data::get().isNonLimitedAdmin())
		{
			// ShellExecuteA ShellExecuteW ShellExecuteExA 测试发现全部会调用ShellExecuteExW 只hook这一个就行
			create_hook_by_func_ptr<&::ShellExecuteExW>().setHookFromGetter([&](auto trampolineConst)
			{
				return HookInfo{&ShellExecuteExW<trampolineConst.value>};
			});
		}
	}
}
