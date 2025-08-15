module;
#define NOMINMAX
#include <shobjidl_core.h>
module UI.StartAppDiv;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;
import Launcher;

namespace
{
	constexpr float MARGIN = 12.f;
	constexpr float PADDING = 12.f;
}

namespace ui
{
	void StartAppDiv::initialize()
	{
		m_btnStart = std::make_unique<Button>(this);
		m_btnStart->setText(L"启动进程");

		m_btnStart->setBackgroundColor(D2D1::ColorF(0x006cbd));
		m_btnStart->setBackgroundColor(D2D1::ColorF(0x0078d4), Button::EState::Normal);
		m_btnStart->setTextColor(D2D1::ColorF(D2D1::ColorF::White));
		m_btnStart->setOnClick([this] { onBtnStartPressed(); });
	}

	void StartAppDiv::onResize(float width, float height)
	{
		const float btnYPos = m_pathTextHeight ? PADDING * 2.f + m_pathTextHeight + MARGIN : 0.f;
		m_btnStart->setBounds(D2D1::RectF(0, btnYPos, width, btnYPos + 36.f));
	}

	void StartAppDiv::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		m_btnStart->draw(renderCtx);
	}

	void StartAppDiv::onBtnStartPressed()
	{
		m_strExePath.clear();
		UniqueComPtr<IFileOpenDialog> fileOpen;
		HResult hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&fileOpen));
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"创建文件选择对话框失败! CoCreateInstance fail, HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return;
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
		hr = fileOpen->Show(m_ownerWnd->nativeHandle());
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"显示文件选择对话框失败! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return;
		}
		UniqueComPtr<IShellItem> item;
		hr = fileOpen->GetResult(&item);
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"无法获取选择的文件! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return;
		}
		PWSTR pszFilePath;
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"无法获取选择的文件路径! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return;
		}
		m_strExePath = pszFilePath;
		CoTaskMemFree(pszFilePath);

		if (SUCCEEDED(app().dWriteFactory()->CreateTextLayout(m_strExePath.c_str(),
			static_cast<UINT32>(m_strExePath.size()),
			app().textFormat().pTipsFormat,
			std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
			&m_pExePathTextLayout)))
		{
			m_pExePathTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
		}

		biz::Launcher::instance().run(m_strExePath);
	}
}
