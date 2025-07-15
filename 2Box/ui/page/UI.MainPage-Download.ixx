export module UI.MainPage:Download;

import "sys_defs.h";
import MainApp;
import StateMachine;
import UI.PageBase;

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

		virtual HRESULT onCreateDeviceResources(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, _] = renderCtx;
			HRESULT hr;
			do
			{
				D2D1_GRADIENT_STOP gradientStops[2] = {
					{0.0f, D2D1::ColorF(0x4A9DF8)},
					{1.0f, D2D1::ColorF(0x6AC0FF)}
				};
				hr = renderTarget->CreateGradientStopCollection(
					gradientStops,
					2,
					D2D1_GAMMA_2_2,
					D2D1_EXTEND_MODE_CLAMP,
					&m_pGradientStops);
				if (FAILED(hr))
				{
					break;
				}

				hr = renderTarget->CreateLinearGradientBrush(
					D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(0, 0)),
					m_pGradientStops,
					&m_pProgressBrush);
				if (FAILED(hr))
				{
					break;
				}
			}
			while (false);

			if (FAILED(hr))
			{
				onDiscardDeviceResources();
			}
			return hr;
		}

		virtual void onDiscardDeviceResources() override
		{
			safe_release(&m_pProgressBrush);
			safe_release(&m_pGradientStops);
		}

		virtual void render(const RenderContext& renderCtx) override
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
			renderTarget->Clear(D2D1::ColorF(0xF5F7FA));
			auto size = renderTarget->GetSize();

			float dpix = 96.f;
			float dpiy = 96.f;
			renderTarget->GetDpi(&dpix, &dpiy);
			float physicalToDevice = dpiy != 0.f ? 96 / dpiy : 1.f;
			const float progressBarWidth = size.width / 2;
			const float progressBarHeight = 25 * physicalToDevice;
			// 进度条位置(居中)
			float barX = (size.width - progressBarWidth) / 2;
			float barY = (size.height - progressBarHeight) / 2;

			// 绘制背景轨道
			D2D1_ROUNDED_RECT trackRect = D2D1::RoundedRect(
				D2D1::RectF(barX, barY, barX + progressBarWidth, barY + progressBarHeight),
				4.0f, 4.0f);
			solidBrush->SetColor(D2D1::ColorF(0xE0E5EC));
			renderTarget->FillRoundedRectangle(&trackRect, solidBrush);

			m_pProgressBrush->SetEndPoint(D2D1::Point2F(progressBarWidth, 0));

			// 绘制进度填充
			float filledWidth = 0.45f * progressBarWidth;
			D2D1_ROUNDED_RECT fillRect = D2D1::RoundedRect(
				D2D1::RectF(barX, barY, barX + filledWidth, barY + progressBarHeight),
				4.0f, 4.0f);
			renderTarget->FillRoundedRectangle(&fillRect, m_pProgressBrush);

			// 绘制文本标签
			std::wstring progressText = std::format(L"任务进度: {}%", 45);

			D2D1_RECT_F textRect = D2D1::RectF(barX, barY - 30 * physicalToDevice, barX + progressBarWidth, barY);
			solidBrush->SetColor(D2D1::ColorF(0x333333));
			renderTarget->DrawTextW(progressText.c_str(),
			                        static_cast<UINT32>(progressText.length()),
			                        m_pTextFormat,
			                        &textRect,
			                        solidBrush);
		}

	private:
		ID2D1GradientStopCollection* m_pGradientStops{nullptr};
		ID2D1LinearGradientBrush* m_pProgressBrush{nullptr};
		IDWriteTextFormat* m_pTextFormat{nullptr};
	};
}
