export module UI.MainWindow;

import UI.WindowBase;

namespace ui
{
	export class MainWindow final : public WindowBase
	{
	public:
		MainWindow();
	};

	export MainWindow* g_main_wnd{nullptr};

	export MainWindow& main_wnd()
	{
		return *g_main_wnd;
	}
}
