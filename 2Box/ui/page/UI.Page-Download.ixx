export module UI.Page:Download;

import "sys_defs.h";
import std;
import MainApp;
import Coroutine;
import Scheduler;
import SymbolLoader;
import UI.Core;
import UI.FileStatusCtrl;

namespace ui
{
	namespace page_dl_detail
	{
		template <ArchBit BitType = CURRENT_ARCH_BIT>
		void initialize_file_status_ctrl(std::unique_ptr<FileStatusCtrl>& ctrl, WindowBase* owner)
		{
			static constexpr std::wstring_view downloadServerName = L"msdl.microsoft.com";
			static std::wstring searchPath{std::format(L"{}\\Symbols", app().exeDir())};

			ctrl = std::make_unique<FileStatusCtrl>(owner);
			ctrl->setArchBit(BitType);
			const auto [pdbDir, pdbName, objName] = symbols::get_ntdll_pdb_info<BitType>(searchPath);
			ctrl->setFilePath(pdbDir, pdbName);
			ctrl->setDownloadUrl(downloadServerName, objName);
			ctrl->startAnaTask();
		}
	}

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

		virtual HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override
		{
			HResult hr = m_p32FileStatusCtrl->onCreateDeviceResources(renderTarget);
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				if (SUCCEEDED(hr))
				{
					hr = m_p64FileStatusCtrl->onCreateDeviceResources(renderTarget);
				}
			}
			return hr;
		}

		virtual void onDiscardDeviceResources() override
		{
			m_p32FileStatusCtrl->onDiscardDeviceResources();
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				m_p64FileStatusCtrl->onDiscardDeviceResources();
			}
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

			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				contentYPos += fileCtrl32Height + bottomMargin;
				const float fileCtrl64Height = m_p64FileStatusCtrl->preferredHeight();
				m_p64FileStatusCtrl->setBounds(D2D1::RectF(contentXPos, contentYPos, contentXPos + contentWidth, contentYPos + fileCtrl64Height));
				m_p64FileStatusCtrl->draw(renderCtx);
			}
		}

		coro::LazyTask<void> joinAsync() const
		{
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				co_await coro::when_all(m_p64FileStatusCtrl->joinAsync(), m_p32FileStatusCtrl->joinAsync());
			}
			else
			{
				co_await m_p32FileStatusCtrl->joinAsync();
			}
			co_return;
		}

		coro::LazyTask<void> untilSuccess() const
		{
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				co_await coro::when_all(m_p64FileStatusCtrl->untilSuccess(), m_p32FileStatusCtrl->untilSuccess());
			}
			else
			{
				co_await m_p32FileStatusCtrl->untilSuccess();
			}
			co_return;
		}

		bool isFileVerified() const
		{
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				return m_p64FileStatusCtrl->isFileVerified() && m_p32FileStatusCtrl->isFileVerified();
			}
			else
			{
				return m_p32FileStatusCtrl->isFileVerified();
			}
		}

		void cancelTask() const
		{
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				m_p64FileStatusCtrl->cancelTask();
			}
			m_p32FileStatusCtrl->cancelTask();
		}

		bool isCancelled() const
		{
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				return m_p64FileStatusCtrl->isCancelled() && m_p32FileStatusCtrl->isCancelled();
			}
			else
			{
				return m_p32FileStatusCtrl->isCancelled();
			}
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


		void initFileStatusCtrl()
		{
			page_dl_detail::initialize_file_status_ctrl<ArchBit::Bit32>(m_p32FileStatusCtrl, m_ownerWnd);
			if constexpr (IS_CURRENT_ARCH_64_BIT)
			{
				page_dl_detail::initialize_file_status_ctrl<ArchBit::Bit64>(m_p64FileStatusCtrl, m_ownerWnd);
			}
		}

	private:
		WindowBase* m_ownerWnd{nullptr};
		UniqueComPtr<IDWriteTextLayout> m_pTextLayout;
		std::unique_ptr<FileStatusCtrl> m_p32FileStatusCtrl{nullptr};
		std::unique_ptr<FileStatusCtrl> m_p64FileStatusCtrl{nullptr};
	};
}
