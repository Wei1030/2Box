module;
#define NOMINMAX
#include <shobjidl_core.h>
module UI.LeftSidebar;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;

namespace
{
	constexpr float PADDING = 24.f;
	constexpr float MARGIN_BOTTOM = 24.f;
}

namespace ui
{
	std::optional<std::wstring> LeftSidebar::selectProcess() const
	{
		UniqueComPtr<IFileOpenDialog> fileOpen;
		HResult hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&fileOpen));
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
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
		hr = fileOpen->Show(m_ownerWnd->nativeHandle());
		if (FAILED(hr))
		{
			if (hr != HRESULT_FROM_WIN32(ERROR_CANCELLED))
			{
				MessageBoxW(m_ownerWnd->nativeHandle(),
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
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"无法获取选择的文件! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return std::nullopt;
		}
		PWSTR pszFilePath;
		hr = item->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
		if (FAILED(hr))
		{
			MessageBoxW(m_ownerWnd->nativeHandle(),
			            std::format(L"无法获取选择的文件路径! HRESULT:{:#08x}", static_cast<std::uint32_t>(hr)).c_str(),
			            MainApp::appName.data(),
			            MB_OK | MB_ICONERROR | MB_TASKMODAL);
			return std::nullopt;
		}
		std::wstring procFullPath{pszFilePath};
		CoTaskMemFree(pszFilePath);
		return procFullPath;
	}

	void LeftSidebar::onResize(float width, float height)
	{
		resizeEnvCardsArea();
	}

	void LeftSidebar::initialize()
	{
		m_startAppDiv = std::make_unique<StartAppDiv>(this);
		m_startAppDiv->setBounds(D2D1::RectF(PADDING, PADDING, PADDING + 232.f, PADDING + 36.f));
		m_startAppDiv->setSizeChangeNotify([this](float, float)
		{
			resizeEnvCardsArea();
		});

		m_envCardsArea = std::make_unique<EnvBoxCardArea>(this);
	}

	void LeftSidebar::resizeEnvCardsArea() const
	{
		const auto contentSize = size();
		const float startYPos = m_startAppDiv->getBounds().bottom + MARGIN_BOTTOM * 2;
		m_envCardsArea->setBounds(D2D1::RectF(PADDING - EnvBoxCardArea::shadowSize,
		                                      startYPos - EnvBoxCardArea::shadowSize + EnvBoxCardArea::shadowOffsetY,
		                                      contentSize.width - PADDING + EnvBoxCardArea::scrollAreaWidth + EnvBoxCardArea::shadowSize,
		                                      contentSize.height));
	}

	void LeftSidebar::drawImpl(const RenderContext& renderCtx)
	{
		const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
		const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
		const auto drawSize = size();

		solidBrush->SetColor(D2D1::ColorF(0xFFFFFF));
		renderTarget->FillRectangle(D2D1::RectF(0.f, 0.f, drawSize.width, drawSize.height), solidBrush);

		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawLine(D2D1::Point2F(drawSize.width, 0.f), D2D1::Point2F(drawSize.width, drawSize.height), solidBrush);

		m_startAppDiv->draw(renderCtx);

		if (m_envCardsArea->isNoEnvs())
		{
			return;
		}

		const float startYPos = m_startAppDiv->getBounds().bottom + MARGIN_BOTTOM;
		solidBrush->SetColor(D2D1::ColorF(0xe0e0e0));
		renderTarget->DrawLine(D2D1::Point2F(PADDING, startYPos), D2D1::Point2F(drawSize.width - PADDING, startYPos), solidBrush);

		m_envCardsArea->draw(renderCtx);
	}
}
