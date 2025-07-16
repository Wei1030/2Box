export module UI.MainPage:Download;

import "sys_defs.h";
import MainApp;
import StateMachine;
import UI.PageBase;
import UI.LoadingIndicator;
import :Define;


namespace ui
{
	export
	template <>
	class TMainPageType<MainPageType, MainPageType::Download> final : public PageBase
	{
	public:
		void OnEnter(WindowBase&)
		{
			app().dWriteFactory()->CreateTextFormat(
				L"Segoe UI",
				nullptr,
				DWRITE_FONT_WEIGHT_NORMAL,
				DWRITE_FONT_STYLE_NORMAL,
				DWRITE_FONT_STRETCH_NORMAL,
				14.0f,
				L"",
				&m_pTextFormat);
		}

		sm::TNextState<MainPageType> OnUpdate(WindowBase&)
		{
			return {MainPageType::Download};
		}

		void OnExit(WindowBase&)
		{
			safe_release(&m_pTextFormat);
		}

		virtual WindowBase::HResult onCreateDeviceResources(const RenderContext& renderCtx) override
		{
			return m_loadingIndicator.onCreateDeviceResources(renderCtx);
		}

		virtual void onDiscardDeviceResources() override
		{
			m_loadingIndicator.onDiscardDeviceResources();
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			renderTarget->Clear(D2D1::ColorF(0xF5F7FA));
			auto size = renderTarget->GetSize();

			const float progressBarWidth = size.width / 2;
			constexpr float progressBarHeight = 8.f;
			// 进度条位置(居中)
			float barX = (size.width - progressBarWidth) / 2;
			float barY = (size.height - progressBarHeight) / 2;

			// 绘制背景轨道
			D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
				D2D1::RectF(barX, barY, barX + progressBarWidth, barY + progressBarHeight),
				4.f, 4.f);
			solidBrush->SetColor(D2D1::ColorF(0xE0E5EC));
			renderTarget->FillRoundedRectangle(&trackRect, solidBrush);

			// 绘制进度填充
			float filledWidth = 0.45f * progressBarWidth;
			m_loadingIndicator.setBounds(D2D1::RectF(barX, barY, barX + filledWidth, barY + progressBarHeight));
			m_loadingIndicator.draw(renderCtx);

			// 绘制文本标签
			std::wstring progressText = std::format(L"任务进度: {}%", 45);

			D2D1_RECT_F textRect = D2D1::RectF(barX, barY - 30, barX + progressBarWidth, barY);
			solidBrush->SetColor(D2D1::ColorF(0x333333));
			renderTarget->DrawTextW(progressText.c_str(),
			                        static_cast<UINT32>(progressText.length()),
			                        m_pTextFormat,
			                        &textRect,
			                        solidBrush);
		}

	private:
		IDWriteTextFormat* m_pTextFormat{nullptr};
		LoadingIndicator m_loadingIndicator;
	};
}
