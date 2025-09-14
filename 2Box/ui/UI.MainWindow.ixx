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
		virtual void onResize(float width, float height) override;
		virtual void onActivate(WParam wParam, LParam lParam) override;
		virtual bool onClose() override;
		virtual bool onNcCalcSize(WParam wParam, LParam lParam) override;
		virtual LResult onNcHitTest(WPARAM wParam, LParam lParam, LResult dwmProcessedResult) override;
		virtual void onNcPaint(WParam wParam, LParam lParam) override;
		virtual void onDwmCompositionChanged() override;

	private:
		void initWindow();
		void reinitWindow();
		void initWindowPosition();
		void initTitleIcon();
		ID2D1Bitmap* getTitleIconBitmap(ID2D1HwndRenderTarget* renderTarget);
		coro::LazyTask<void> initSymbols();

	private:
		template <typename PageType>
		void changePageTo()
		{
			m_pages = std::make_unique<PageType>(this);
			if (isCompositionEnabled())
			{
				getPage<PageType>().setMargins(m_margins);
			}
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

	private:
		MARGINS m_physicalMargins{};
		D2D1_RECT_F m_margins{};
		BITMAP m_bmIcon;
		std::vector<std::byte> m_bmpIconData;
		UniqueComPtr<ID2D1Bitmap> m_pD2D1Bitmap;
		D2D1_COLOR_F m_titleColor{};
		UniqueComPtr<IDWriteTextLayout> m_pTitleLayout;
		float m_titleTextHeight{};
		float m_captionBtnWidth{};
	};

	export MainWindow* g_main_wnd{nullptr};

	export MainWindow& main_wnd()
	{
		return *g_main_wnd;
	}
}
