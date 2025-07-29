export module UI.MainWindow;

import "sys_defs.h";
import std;
import UI.Core;
import UI.Page;
import Coroutine;

namespace ui
{
	export class MainWindow final : public WindowBase
	{
	public:
		MainWindow();

	public:
		virtual HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override;
		virtual void onDiscardDeviceResources() override;
		virtual void draw(const RenderContext& renderCtx) override;

	protected:
		virtual bool onClose() override;

	private:
		void initWindowPosition();
		coro::LazyTask<void> initSymbols();

	private:
		template <typename PageType>
		void changePageTo()
		{
			m_pages = std::make_unique<PageType>(this);
			requestCreateDeviceResources();
			invalidateRect();
		}

		template <typename PageType>
		bool isPage() const
		{
			return std::holds_alternative<std::unique_ptr<PageType>>(m_pages);
		}

		template <typename PageType>
		PageType& getPage() const
		{
			if (std::holds_alternative<std::unique_ptr<PageType>>(m_pages))
			{
				return *std::get<std::unique_ptr<PageType>>(m_pages).get();
			}
			throw std::runtime_error("page not exists");
		}

		RendererInterface* currentRenderer() const
		{
			if (std::holds_alternative<std::unique_ptr<DownloadPage>>(m_pages))
			{
				return std::get<std::unique_ptr<DownloadPage>>(m_pages).get();
			}
			if (std::holds_alternative<std::unique_ptr<HomePage>>(m_pages))
			{
				return std::get<std::unique_ptr<HomePage>>(m_pages).get();
			}
			throw std::runtime_error("unknown state");
		}

	private:
		std::variant<std::monostate, std::unique_ptr<DownloadPage>, std::unique_ptr<HomePage>> m_pages;
	};

	export MainWindow* g_main_wnd{nullptr};

	export MainWindow& main_wnd()
	{
		return *g_main_wnd;
	}
}
