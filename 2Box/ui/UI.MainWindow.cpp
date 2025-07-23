module UI.MainWindow;

import std;
import MainApp;
import Scheduler;

namespace ui
{
	static constexpr int DESIRED_WIDTH = 1024;
	static constexpr int DESIRED_HEIGHT = 768;

	MainWindow::MainWindow() : WindowBase({MainApp::appName})
	{
		setExitAppWhenWindowDestroyed(true);
		initWindowPosition();
		initSymbols().detachAndStart();
	}

	void MainWindow::onResize(const RectChangeContext& ctx)
	{
		resizeAllPages(std::make_integer_sequence<std::uint8_t, static_cast<std::uint8_t>(MainPageType::TotalCount)>(), ctx);
	}

	WindowBase::HResult MainWindow::onCreateDeviceResources()
	{
		return createDeviceResourcesForAllPages(std::make_integer_sequence<std::uint8_t, static_cast<std::uint8_t>(MainPageType::TotalCount)>());
	}

	void MainWindow::onDiscardDeviceResources()
	{
		discardDeviceResourcesForAllPages(std::make_integer_sequence<std::uint8_t, static_cast<std::uint8_t>(MainPageType::TotalCount)>());
	}

	WindowBase::HResult MainWindow::onRender()
	{
		const RenderContext& ctx = renderContext();
		ctx.renderTarget->BeginDraw();
		ctx.renderTarget->PushAxisAlignedClip(rectNeedUpdate(), D2D1_ANTIALIAS_MODE_PER_PRIMITIVE);
		currentPage().draw(ctx);
		ctx.renderTarget->PopAxisAlignedClip();
		return ctx.renderTarget->EndDraw();
	}

	bool MainWindow::onClose()
	{
		if (m_pages.currentStateIndex() == MainPageType::Download)
		{
			auto& downloadPage = m_pages.stateCtx<MainPageType::Download>();
			if (!downloadPage.isStopRequested())
			{
				downloadPage.cancelTask();
				[](MainWindow& self, TMainPageType<MainPageType, MainPageType::Download>& page) -> coro::OnewayTask
				{
					try
					{
						co_await page.joinAsync();
					}
					catch (...)
					{
					}
					co_await sched::transfer_to(app().get_scheduler());
					self.changePageTo<MainPageType::Home>();
					self.destroyWindow();
				}(*this, downloadPage);
			}
			return true;
		}
		return false;
	}

	void MainWindow::initWindowPosition()
	{
		const auto physicalRc = physicalRect();
		const float physicalWidth = physicalRc.right - physicalRc.left;
		const float physicalHeight = physicalRc.bottom - physicalRc.top;

		const float deviceToPhysical = dpiInfo().deviceToPhysical;
		const float desiredPhysicalWidth = std::min(DESIRED_WIDTH * deviceToPhysical, physicalWidth);
		const float desiredPhysicalHeight = std::min(DESIRED_HEIGHT * deviceToPhysical, physicalHeight);

		const float diffWidth = physicalWidth - desiredPhysicalWidth;
		const float diffHeight = physicalHeight - desiredPhysicalHeight;

		const float desiredX = physicalRc.left + diffWidth * 0.5f;
		const float desiredY = physicalRc.top + diffHeight * 0.5f;

		setPhysicalRect(D2D1::RectF(
			desiredX,
			desiredY,
			desiredX + desiredPhysicalWidth,
			desiredY + desiredPhysicalHeight
		));
	}

	coro::LazyTask<void> MainWindow::initSymbols()
	{
		m_pages.setCtx(this);
		changePageTo<MainPageType::Download>();

		co_await m_pages.stateCtx<MainPageType::Download>().joinAsync();
		co_await sched::transfer_to(app().get_scheduler());
		
		changePageTo<MainPageType::Home>();
		co_return;
	}
}
