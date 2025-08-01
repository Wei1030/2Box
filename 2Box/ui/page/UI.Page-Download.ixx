export module UI.Page:Download;

import "sys_defs.h";
import std;
import MainApp;
import Coroutine;
import Scheduler;
import SymbolLoader;
import Utility.SystemInfo;
import UI.Core;
import UI.FileStatusCtrl;

namespace ui
{
	export class DownloadPage final : public RendererInterface
	{
	public:
		static constexpr std::wstring_view downloadPageTitle = L"正在验证必需的PDB符号文件。软件首次启动或系统更新时，可能需要从微软官方下载，请耐心等待。";

		explicit DownloadPage(WindowBase* owner)
		{
			m_ownerWnd = owner;

			initTextLayout();
			initFileStatusCtrl();
		}

		virtual ~DownloadPage()
		{
			m_p32FileStatusCtrl.reset();
#ifdef _WIN64
			m_p64FileStatusCtrl.reset();
#endif
			m_pTextLayout.reset();
		}

		virtual HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override
		{
			HResult hr;
			(hr = m_p32FileStatusCtrl->onCreateDeviceResources(renderTarget), FAILED(hr))
#ifdef _WIN64
				|| (hr = m_p64FileStatusCtrl->onCreateDeviceResources(renderTarget), FAILED(hr))
#endif
				;
			return hr;
		}

		virtual void onDiscardDeviceResources() override
		{
			m_p32FileStatusCtrl->onDiscardDeviceResources();
#ifdef _WIN64
			m_p64FileStatusCtrl->onDiscardDeviceResources();
#endif
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			renderTarget->Clear(D2D1::ColorF(0xF5F7FA));
			auto size = renderTarget->GetSize();

			constexpr float bottomMargin = 20.f;
			const float contentWidth = size.width * 0.618f;
			float titleTextHeight = 24.f;
			if (m_pTextLayout)
			{
				DWRITE_TEXT_METRICS textMetrics;
				if (SUCCEEDED(m_pTextLayout->SetMaxWidth(contentWidth))
					&& SUCCEEDED(m_pTextLayout->GetMetrics(&textMetrics)))
				{
					titleTextHeight = textMetrics.height;
				}
			}

			const float contentXPos = (size.width - contentWidth) / 2;
			float contentYPos = bottomMargin;
			solidBrush->SetColor(D2D1::ColorF(0x2b579a));
			renderTarget->DrawTextW(downloadPageTitle.data(),
			                        static_cast<UINT32>(downloadPageTitle.length()),
			                        app().textFormat().pTitleFormat,
			                        D2D1::RectF(contentXPos, contentYPos, contentXPos + contentWidth, contentYPos + titleTextHeight),
			                        solidBrush);
			contentYPos += titleTextHeight + bottomMargin;

			const float fileCtrl32Height = m_p32FileStatusCtrl->preferredHeight();
			m_p32FileStatusCtrl->setBounds(D2D1::RectF(contentXPos, contentYPos, contentXPos + contentWidth, contentYPos + fileCtrl32Height));
			m_p32FileStatusCtrl->draw(renderCtx);
			contentYPos += fileCtrl32Height + bottomMargin;
#ifdef _WIN64
			const float fileCtrl64Height = m_p64FileStatusCtrl->preferredHeight();
			m_p64FileStatusCtrl->setBounds(D2D1::RectF(contentXPos, contentYPos, contentXPos + contentWidth, contentYPos + fileCtrl64Height));
			m_p64FileStatusCtrl->draw(renderCtx);
#endif
		}

		coro::LazyTask<void> joinAsync() const
		{
#ifdef _WIN64
			co_await coro::when_all(m_p64FileStatusCtrl->joinAsync(), m_p32FileStatusCtrl->joinAsync());
#else
			co_await m_p32FileStatusCtrl->joinAsync();
#endif
			co_return;
		}

		coro::LazyTask<void> untilSuccess() const
		{
#ifdef _WIN64
			co_await coro::when_all(m_p64FileStatusCtrl->untilSuccess(), m_p32FileStatusCtrl->untilSuccess());
#else
			co_await m_p32FileStatusCtrl->untilSuccess();
#endif
			co_return;
		}

		bool isFileVerified() const
		{
#ifdef _WIN64
			return m_p64FileStatusCtrl->isFileVerified() && m_p32FileStatusCtrl->isFileVerified();
#else
			return m_p32FileStatusCtrl->isFileVerified();
#endif
		}

		void cancelTask() const
		{
#ifdef _WIN64
			m_p64FileStatusCtrl->cancelTask();
#endif
			m_p32FileStatusCtrl->cancelTask();
		}

		bool isCancelled() const
		{
#ifdef _WIN64
			return m_p64FileStatusCtrl->isCancelled() && m_p32FileStatusCtrl->isCancelled();
#else
			return m_p32FileStatusCtrl->isCancelled();
#endif
		}

	private:
		void initTextLayout()
		{
			if (SUCCEEDED(app().dWriteFactory()->CreateTextLayout(downloadPageTitle.data(),
				static_cast<UINT32>(downloadPageTitle.size()),
				app().textFormat().pTitleFormat,
				std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
				&m_pTextLayout)))
			{
				m_pTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
			}
		}

		static constexpr std::wstring_view downloadServerName = L"msdl.microsoft.com";

		void initFileStatusCtrl()
		{
			namespace fs = std::filesystem;
			static std::wstring searchPath{std::format(L"{}\\Symbols", app().exeDir())};

			{
				m_p32FileStatusCtrl = std::make_unique<FileStatusCtrl>(m_ownerWnd);
#ifdef _WIN64
				m_p32FileStatusCtrl->setIs32(true);
				const fs::path systemDir{sys_info::get_system_wow64_dir()};
#else
				const fs::path systemDir{sys_info::get_system_dir()};
#endif
				const fs::path ntdllPath{fs::weakly_canonical(systemDir / fs::path{L"ntdll.dll"})};
				const SYMSRV_INDEX_INFOW indexInfo = symbols::file_index_info(ntdllPath.native());
				const auto [pdbDir, objName] = symbols::parse_pdb_path(indexInfo, searchPath);
				m_p32FileStatusCtrl->setFilePath(pdbDir, indexInfo.pdbfile);
				m_p32FileStatusCtrl->setDownloadUrl(downloadServerName, objName);
				m_p32FileStatusCtrl->startAnaTask();
			}
			{
#ifdef _WIN64
				m_p64FileStatusCtrl = std::make_unique<FileStatusCtrl>(m_ownerWnd);
				const fs::path systemDir{sys_info::get_system_dir()};
				const fs::path ntdllPath{fs::weakly_canonical(systemDir / fs::path{L"ntdll.dll"})};
				const SYMSRV_INDEX_INFOW indexInfo = symbols::file_index_info(ntdllPath.native());
				const auto [pdbDir, objName] = symbols::parse_pdb_path(indexInfo, searchPath);
				m_p64FileStatusCtrl->setFilePath(pdbDir, indexInfo.pdbfile);
				m_p64FileStatusCtrl->setDownloadUrl(downloadServerName, objName);
				m_p64FileStatusCtrl->startAnaTask();
#endif
			}
		}

	private:
		WindowBase* m_ownerWnd{nullptr};
		UniqueComPtr<IDWriteTextLayout> m_pTextLayout;
		std::unique_ptr<FileStatusCtrl> m_p32FileStatusCtrl{nullptr};
#ifdef _WIN64
		std::unique_ptr<FileStatusCtrl> m_p64FileStatusCtrl{nullptr};
#endif
	};
}
