module UI.MainWindow;
import MainApp;

namespace ui
{
	MainWindow::MainWindow() : WindowBase({MainApp::appName})
	{
		setExitAppWhenWindowDestroyed(true);
	}
}
