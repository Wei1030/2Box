export module UI.MainWindow;

import UI.WindowBase;


namespace ui
{
	export class MainWindow final : public WindowBase
	{
	public:
		MainWindow();
	};

	export inline MainWindow* g_main_wnd = nullptr;
}
