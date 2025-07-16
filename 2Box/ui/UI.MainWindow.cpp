module UI.MainWindow;

import std;
import MainApp;

namespace ui
{
	// coro::SharedTask<void> testCoro()
	// {
	// 	std::uint64_t currentSize = 0;
	// 	std::uint64_t totalSize = 0;
	// 	symbols::Loader loader{ std::format(L"{}\\Symbols", MainApp::getDir()) };
	// 	symbols::Symbol sym = co_await loader.loadNtdllSymbol(
	// 		[&](std::uint64_t ts)->coro::SharedTask<void> {
	// 			co_await  sched::transfer_to(MainApp::get_scheduler());
	// 			totalSize = ts;
	// 			OutputDebugStringW(std::format(L"total size: {} \r\n", totalSize).c_str());
	// 		},
	// 		[&](std::uint64_t size)->coro::SharedTask<void> {
	// 			co_await sched::transfer_to(MainApp::get_scheduler());
	// 			currentSize += size;
	// 			OutputDebugStringW(std::format(L"{} / {} \r\n", currentSize, totalSize).c_str());
	// 		}
	// 	);
	// 	co_return;
	// }

	static constexpr int DESIRED_WIDTH = 640;
	static constexpr int DESIRED_HEIGHT = 480;

	MainWindow::MainWindow() : WindowBase({MainApp::appName})
	{
		setExitAppWhenWindowDestroyed(true);
		initWindowPosition();

		m_pages.setCtx(this);
		changePageTo<MainPageType::Download>();
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
		currentPage().draw(ctx);
		return ctx.renderTarget->EndDraw();
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
}
