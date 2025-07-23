export module UI.MainPage:Download;

import "sys_defs.h";
import std;
import :Define;
import MainApp;
import StateMachine;
import Coroutine;
import Scheduler;
import SymbolLoader;
import UI.PageBase;
import UI.FileStatusCtrl;

namespace ui
{
	export
	template <>
	class TMainPageType<MainPageType, MainPageType::Download> final : public PageBase
	{
	public:
		static constexpr std::wstring_view downloadPageTitle = L"正在验证必需的PDB符号文件。软件首次启动或系统更新时，可能需要从微软官方下载，请耐心等待。";

		void OnEnter(WindowBase& owner)
		{
			m_ownerWnd = &owner;
			if (SUCCEEDED(app().dWriteFactory()->CreateTextLayout(downloadPageTitle.data(),
				static_cast<UINT32>(downloadPageTitle.size()),
				app().textFormat().pTitleFormat,
				std::numeric_limits<float>::max(), std::numeric_limits<float>::max(),
				&m_pTextLayout)))
			{
				m_pTextLayout->SetWordWrapping(DWRITE_WORD_WRAPPING_WRAP);
			}

			m_p32FileStatusCtrl = new FileStatusCtrl(&owner);
			m_p64FileStatusCtrl = new FileStatusCtrl(&owner);

			//m_task = coro::start_and_shared(coro::co_with_cancellation(downloadIfNeedThenAnaSymbols(), m_stopSource.get_token()));
		}

		sm::TNextState<MainPageType> OnUpdate(WindowBase&)
		{
			return {MainPageType::Download};
		}

		void OnExit(WindowBase&)
		{
			m_stopSource.request_stop();
			m_task.waitUntilDone();
			m_asyncScope.join();
			delete m_p32FileStatusCtrl;
			delete m_p64FileStatusCtrl;
			safe_release(&m_pTextLayout);
		}

		virtual WindowBase::HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override
		{
			WindowBase::HResult hr;
			(hr = m_p32FileStatusCtrl->onCreateDeviceResources(renderTarget), FAILED(hr)) || (hr = m_p64FileStatusCtrl->onCreateDeviceResources(renderTarget), FAILED(hr));
			return hr;
		}

		virtual void onDiscardDeviceResources() override
		{
			m_p32FileStatusCtrl->onDiscardDeviceResources();
			m_p64FileStatusCtrl->onDiscardDeviceResources();
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			renderTarget->Clear(D2D1::ColorF(0xF5F7FA));
			auto size = renderTarget->GetSize();

			constexpr float bottomMargin = 20.f;
			const float contentWidth = size.width / 2;
			const float fileCtrl32Height = m_p32FileStatusCtrl->preferredHeight();
			const float fileCtrl64Height = m_p64FileStatusCtrl->preferredHeight();
			const float contentHeight = fileCtrl32Height + bottomMargin + fileCtrl64Height;
			const float contentXPos = (size.width - contentWidth) / 2;
			float contentYPos = (size.height - contentHeight) / 2;

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
			const float titleYPos = contentYPos - bottomMargin - titleTextHeight;
			solidBrush->SetColor(D2D1::ColorF(0x2b579a));
			renderTarget->DrawTextW(downloadPageTitle.data(),
			                        static_cast<UINT32>(downloadPageTitle.length()),
			                        app().textFormat().pTitleFormat,
			                        D2D1::RectF(contentXPos, titleYPos, contentXPos + contentWidth, titleYPos + titleTextHeight),
			                        solidBrush);

			m_p32FileStatusCtrl->setBounds(D2D1::RectF(contentXPos, contentYPos, contentXPos + contentWidth, contentYPos + fileCtrl32Height));
			m_p32FileStatusCtrl->draw(renderCtx);

			contentYPos += fileCtrl32Height + bottomMargin;
			m_p64FileStatusCtrl->setBounds(D2D1::RectF(contentXPos, contentYPos, contentXPos + contentWidth, contentYPos + fileCtrl64Height));
			m_p64FileStatusCtrl->draw(renderCtx);
		}

		coro::LazyTask<void> joinAsync()
		{
			co_await m_task;
			co_return;
		}

		bool isStopRequested() const
		{
			return m_stopSource.stop_requested();
		}

		void cancelTask()
		{
			m_stopSource.request_stop();
		}

	private:
		coro::LazyTask<void> updateTotalSizeInMainThread(std::uint64_t total)
		{
			co_await sched::transfer_to(app().get_scheduler());
			m_totalLength += total;
			m_ownerWnd->invalidateRect();
			co_return;
		}

		coro::LazyTask<void> updateCurrentSizeInMainThread(std::uint64_t size)
		{
			co_await sched::transfer_to(app().get_scheduler());
			m_currentSize += size;
			m_ownerWnd->invalidateRect();
			co_return;
		}

		coro::LazyTask<void> downloadIfNeedThenAnaSymbols()
		{
			auto totalSizeCallback = [this](std::uint64_t total)
			{
				m_asyncScope.spawn(updateTotalSizeInMainThread(total));
			};
			auto currentSizeCallback = [this](std::uint64_t size)
			{
				m_asyncScope.spawn(updateCurrentSizeInMainThread(size));
			};

			symbols::Loader loader{std::format(L"{}\\Symbols", app().exeDir())};
			auto task1 = loader.loadNtdllSymbol(totalSizeCallback, currentSizeCallback);
			auto task2 = loader.loadWow64NtdllSymbol(totalSizeCallback, currentSizeCallback);
			try
			{
				const auto [symbol, symbolWow64] = co_await coro::when_all(std::move(task1), std::move(task2));
				co_await sched::transfer_to(app().get_scheduler());
			}
			catch (const std::exception&)
			{
			}
			catch (...)
			{
			}
			co_await sched::transfer_to(app().get_scheduler());
			//
			co_return;
		}

	private:
		WindowBase* m_ownerWnd{nullptr};
		IDWriteTextLayout* m_pTextLayout{nullptr};
		FileStatusCtrl* m_p32FileStatusCtrl{nullptr};
		FileStatusCtrl* m_p64FileStatusCtrl{nullptr};

	private:
		std::uint64_t m_totalLength{0};
		std::uint64_t m_currentSize{0};
		coro::AsyncScope m_asyncScope;
		std::stop_source m_stopSource;
		coro::SharedTask<void> m_task{coro::SharedTask<void>::reject("task not started")};
	};
}
