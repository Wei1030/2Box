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
import UI.LoadingIndicator;

namespace ui
{
	export
	template <>
	class TMainPageType<MainPageType, MainPageType::Download> final : public PageBase
	{
	public:
		void OnEnter(WindowBase& owner)
		{
			app().dWriteFactory()->CreateTextFormat(
				L"Segoe UI",
				nullptr,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				14.0f,
				L"",
				&m_pTextFormat);

			m_updateWindow = [&owner]
			{
				owner.invalidateRect();
			};
			m_loadingIndicator.setAnimRequester([&owner](const D2D1_RECT_F& rc)
			{
				owner.invalidateRect(rc);
			});
			m_task = coro::start_and_shared(coro::co_with_cancellation(downloadIfNeedThenAnaSymbols(), m_stopSource.get_token()));
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
			m_loadingIndicator.setAnimRequester(nullptr);
			safe_release(&m_pTextFormat);
		}

		virtual WindowBase::HResult onCreateDeviceResources(const RenderContext& renderCtx) override
		{
			return m_loadingIndicator.onCreateDeviceResources(renderCtx);
		}

		virtual void onDiscardDeviceResources() override
		{
			m_loadingIndicator.onDiscardDeviceResources();
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			renderTarget->Clear(D2D1::ColorF(0xF5F7FA));
			auto size = renderTarget->GetSize();

			const float progressBarWidth = size.width / 2;
			constexpr float progressBarHeight = 8.f;
			// 进度条位置(居中)
			float barX = (size.width - progressBarWidth) / 2;
			float barY = (size.height - progressBarHeight) / 2;

			// 绘制背景轨道
			D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
				D2D1::RectF(barX, barY, barX + progressBarWidth, barY + progressBarHeight),
				4.f, 4.f);
			solidBrush->SetColor(D2D1::ColorF(0xE0E5EC));
			renderTarget->FillRoundedRectangle(&trackRect, solidBrush);

			// 绘制进度填充
			const float progress = m_totalLength ? static_cast<float>(m_currentSize) / m_totalLength : 0.f;
			float filledWidth = progress * progressBarWidth;
			m_loadingIndicator.setBounds(D2D1::RectF(barX, barY, barX + filledWidth, barY + progressBarHeight));
			m_loadingIndicator.draw(renderCtx);

			// 绘制文本标签
			std::wstring progressText = std::format(L"任务进度: {:.0f}%", progress * 100);

			D2D1_RECT_F textRect = D2D1::RectF(barX, barY - 30, barX + progressBarWidth, barY);
			solidBrush->SetColor(D2D1::ColorF(0x333333));
			renderTarget->DrawTextW(progressText.c_str(),
			                        static_cast<UINT32>(progressText.length()),
			                        m_pTextFormat,
			                        &textRect,
			                        solidBrush);
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
			m_updateWindow();
			co_return;
		}

		coro::LazyTask<void> updateCurrentSizeInMainThread(std::uint64_t size)
		{
			co_await sched::transfer_to(app().get_scheduler());
			m_currentSize += size;
			m_updateWindow();
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
		IDWriteTextFormat* m_pTextFormat{nullptr};
		LoadingIndicator m_loadingIndicator;

	private:
		std::uint64_t m_totalLength{0};
		std::uint64_t m_currentSize{0};
		coro::AsyncScope m_asyncScope;
		std::stop_source m_stopSource;
		coro::SharedTask<void> m_task{coro::SharedTask<void>::reject(std::make_exception_ptr(std::runtime_error("task not started")))};

		std::move_only_function<void()> m_updateWindow;
	};
}
