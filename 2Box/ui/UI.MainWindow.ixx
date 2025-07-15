export module UI.MainWindow;

import "sys_defs.h";
import std;
import UI.WindowBase;
import UI.PageBase;
import UI.MainPage;
import StateMachine;

namespace ui
{
	export class MainWindow final : public WindowBase
	{
	public:
		MainWindow();

	protected:
		virtual void onResize(const RectChangeContext& ctx) override;
		
		virtual HResult onCreateDeviceResources() override;

		virtual void onDiscardDeviceResources() override;

		virtual HResult onRender() override;

	private:
		template <MainPageType PageType>
		void changePageTo()
		{
			m_pages.transferTo<PageType>();
			m_pages.update();
		}

		PageBase& currentPage()
		{
			if (m_pages.currentStateIndex() == MainPageType::Download)
			{
				return m_pages.stateCtx<MainPageType::Download>();
			}

			if (m_pages.currentStateIndex() == MainPageType::Home)
			{
				return m_pages.stateCtx<MainPageType::Home>();
			}

			throw std::runtime_error("Unknown state");
		}

		template <std::uint8_t... Is>
		void resizeAllPages(std::integer_sequence<std::uint8_t, Is...>, const RectChangeContext& ctx)
		{
			(m_pages.stateCtx<static_cast<MainPageType>(Is)>().onResize(ctx), ...);
		}

		template <std::uint8_t... Is>
		HResult createDeviceResourcesForAllPages(std::integer_sequence<std::uint8_t, Is...>)
		{
			HResult hr;
			((hr = m_pages.stateCtx<static_cast<MainPageType>(Is)>().onCreateDeviceResources(renderContext()), FAILED(hr)) || ...);
			return hr;
		}

		template <std::uint8_t... Is>
		void discardDeviceResourcesForAllPages(std::integer_sequence<std::uint8_t, Is...>)
		{
			(m_pages.stateCtx<static_cast<MainPageType>(Is)>().onDiscardDeviceResources(), ...);
		}

	private:
		sm::StateMachine<TMainPageType, MainPageType, WindowBase> m_pages;
	};

	export MainWindow* g_main_wnd{nullptr};

	export MainWindow& main_wnd()
	{
		return *g_main_wnd;
	}
}
