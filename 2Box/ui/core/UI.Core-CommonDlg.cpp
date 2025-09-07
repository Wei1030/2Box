module;
#define NOMINMAX
#include <shobjidl_core.h>
module UI.Core;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif
import MainApp;

namespace ui
{
	std::optional<std::wstring> select_file(const WindowBase* owner)
	{
		UniqueComPtr<IFileOpenDialog> fileOpen;
		HResult hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&fileOpen));
		if (FAILED(hr))
		{
			MessageBoxW(owner->nativeHandle(),
			            std::format(L"创建文件选择对话框失败! CoCreateInstance fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return std::nullopt;
		}
		COMDLG_FILTERSPEC rgSpec[] =
		{
			{L"可执行文件", L"*.exe"},
			{L"所有文件", L"*.*"}
		};
		fileOpen->SetFileTypes(ARRAYSIZE(rgSpec), rgSpec);
		fileOpen->SetFileTypeIndex(1);
		DWORD dwOptions = 0;
		fileOpen->GetOptions(&dwOptions);
		fileOpen->SetOptions(dwOptions | FOS_STRICTFILETYPES | FOS_FORCEFILESYSTEM);
		hr = fileOpen->Show(owner->nativeHandle());
		if (FAILED(hr))
		{
			if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
			{
				MessageBoxW(owner->nativeHandle(),
				            std::format(L"显示文件选择对话框失败! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
				            MainApp::appName.data(),
				            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			}
			return std::nullopt;
		}
		UniqueComPtr<IShellItem> item;
		hr = fileOpen->GetResult(&item);
		if (FAILED(hr))
		{
			MessageBoxW(owner->nativeHandle(),
			            std::format(L"无法获取选择的文件! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return std::nullopt;
		}
		PWSTR pszFilePath;
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
		if (FAILED(hr))
		{
			MessageBoxW(owner->nativeHandle(),
			            std::format(L"无法获取选择的文件路径! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return std::nullopt;
		}
		std::wstring procFullPath{pszFilePath};
		CoTaskMemFree(pszFilePath);
		return procFullPath;
	}
}
