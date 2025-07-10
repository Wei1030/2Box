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

	MainWindow::MainWindow() : WindowBase({MainApp::appName})
	{
		setExitAppWhenWindowDestroyed(true);

		m_pages.setCtx(this);
		changePageTo<MainPageType::Download>();
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
		return currentPage().render(m_pRenderTarget);
	}
}
