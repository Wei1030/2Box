export module UI.LoadingIndicator;

import "sys_defs.h";
import std;
import Coroutine;
import UI.Core;

namespace ui
{
	export class LoadingIndicator final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

		virtual ~LoadingIndicator()
		{
			stopAnim();
		}

		void startAnim()
		{
			stopAnim();

			m_animStopSource = std::stop_source{};
			m_animTask = coro::start_and_shared(coro::co_with_cancellation(updateShimmerPosition(), m_animStopSource.get_token()));
		}

		void stopAnim()
		{
			m_animStopSource.request_stop();
			m_animTask.waitUntilDone();
		}

	public:
		virtual HResult onCreateDeviceResources(ID2D1HwndRenderTarget* renderTarget) override
		{
			D2D1_GRADIENT_STOP stops[2] = {
				{0.0f, D2D1::ColorF(0x4A9DF8)},
				{1.0f, D2D1::ColorF(0x6AC0FF)}
			};
			UniqueComPtr<ID2D1GradientStopCollection> gradientStops;
			HResult hr = renderTarget->CreateGradientStopCollection(
				stops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&gradientStops);
			if (FAILED(hr))
			{
				return hr;
			}

			hr = renderTarget->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(0, 0)),
				gradientStops,
				&m_pLinearGradientBrush);
			if (FAILED(hr))
			{
				return hr;
			}
			return hr;
		}

		virtual void onDiscardDeviceResources() override
		{
			m_pLinearGradientBrush.reset();
		}

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override
		{
			const UniqueComPtr<ID2D1HwndRenderTarget>& renderTarget = renderCtx.renderTarget;
			const UniqueComPtr<ID2D1SolidColorBrush>& solidBrush = renderCtx.brush;
			const auto drawSize = size();

			// 绘制背景
			m_pLinearGradientBrush->SetEndPoint(D2D1::Point2F(drawSize.width, 0));
			D2D1_ROUNDED_RECT fillRect = D2D1::RoundedRect(
				D2D1::RectF(0, 0, drawSize.width, drawSize.height),
				4.f, 4.f);
			renderTarget->FillRoundedRectangle(&fillRect, m_pLinearGradientBrush);

			// 绘制动画轨道
			D2D1_ROUNDED_RECT shimmerRect = D2D1::RoundedRect(
				D2D1::RectF(m_shimmerPosition, 0, m_shimmerPosition + drawSize.width, drawSize.height),
				4.f, 4.f);
			solidBrush->SetColor(D2D1::ColorF(1.0f, 1.0f, 1.0f, 0.4f));
			renderTarget->FillRoundedRectangle(&shimmerRect, solidBrush);
		}

		coro::LazyTask<void> updateShimmerPosition();

	private:
		UniqueComPtr<ID2D1LinearGradientBrush> m_pLinearGradientBrush;
		std::stop_source m_animStopSource{std::nostopstate};
		coro::SharedTask<void> m_animTask{coro::SharedTask<void>::reject("anim not start")};
		float m_shimmerPosition{0.f};
	};
}
