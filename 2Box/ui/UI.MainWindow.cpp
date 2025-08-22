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
		reserveRenderers(2, 20);
		// initSymbols().detachAndStart();
		changePageTo<HomePage>();
	}

	void MainWindow::draw(const RenderContext& renderCtx)
	{
		currentRenderer()->draw(renderCtx);
	}

	void MainWindow::onResize(float width, float height)
	{
		if (!std::holds_alternative<std::monostate>(m_pages))
		{
			currentRenderer()->onResize(width, height);
		}
	}

	bool MainWindow::onClose()
	{
		if (isPage<DownloadPage>())
		{
			const DownloadPage& downloadPage = getPage<DownloadPage>();
			if (!downloadPage.isCancelled())
			{
				downloadPage.cancelTask();
				[](MainWindow& self, const DownloadPage& page) -> coro::OnewayTask
				{
					co_await page.joinAsync();
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
		changePageTo<DownloadPage>();

		co_await getPage<DownloadPage>().untilSuccess();

		if (getPage<DownloadPage>().isFileVerified())
		{
			changePageTo<HomePage>();
		}
		co_return;
	}
}
