export module UI.LoadingIndicator;

import "sys_defs.h";
import std;
import MainApp;
import UI.ControlBase;
import Coroutine;
import Scheduler;

namespace ui
{
	export class LoadingIndicator final : public ControlTmplBase<LoadingIndicator>
	{
	public:
		LoadingIndicator() : m_animTask(coro::SharedTask<void>::reject(std::make_exception_ptr(std::runtime_error("anim not start"))))
		{
		}

		virtual ~LoadingIndicator()
		{
			stopAnim();
		}

		using AnimRequester = std::move_only_function<void(const D2D1_RECT_F&)>;

		void setAnimRequester(AnimRequester animRequester)
		{
			m_animRequester = std::move(animRequester);
			if (m_animRequester)
			{
				startAnim();
			}
			else
			{
				stopAnim();
			}
		}

	private:
		friend ControlTmplBase;

		WindowBase::HResult createDeviceResourcesImpl(ID2D1HwndRenderTarget* renderTarget)
		{
			D2D1_GRADIENT_STOP gradientStops[2] = {
				{0.0f, D2D1::ColorF(0x4A9DF8)},
				{1.0f, D2D1::ColorF(0x6AC0FF)}
			};
			WindowBase::HResult hr = renderTarget->CreateGradientStopCollection(
				gradientStops,
				2,
				D2D1_GAMMA_2_2,
				D2D1_EXTEND_MODE_CLAMP,
				&m_pGradientStops);
			if (FAILED(hr))
			{
				return hr;
			}

			hr = renderTarget->CreateLinearGradientBrush(
				D2D1::LinearGradientBrushProperties(D2D1::Point2F(0, 0), D2D1::Point2F(0, 0)),
				m_pGradientStops,
				&m_pLinearGradientBrush);
			if (FAILED(hr))
			{
				return hr;
			}
			return hr;
		}

		void discardDeviceResourcesImpl()
		{
			safe_release(&m_pLinearGradientBrush);
			safe_release(&m_pGradientStops);
		}

		void drawImpl(const RenderContext& renderCtx) const
		{
			const auto& [renderTarget, solidBrush] = renderCtx;
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

		coro::LazyTask<void> updateShimmerPosition()
		{
			constexpr long long intervalMs = 1000 / 60;
			constexpr float intervalSec = intervalMs / 1000.f;
			constexpr float speed = 200.f;

			while (true)
			{
				co_await sched::transfer_after(std::chrono::milliseconds{intervalMs}, app().get_scheduler());
				if (!m_animRequester)
				{
					break;
				}

				const float width = size().width;
				m_shimmerPosition += speed * intervalSec;
				if (m_shimmerPosition >= width)
				{
					m_shimmerPosition = -width;
				}

				m_animRequester(m_bounds);
			}
		}

	private:
		ID2D1GradientStopCollection* m_pGradientStops{nullptr};
		ID2D1LinearGradientBrush* m_pLinearGradientBrush{nullptr};
		AnimRequester m_animRequester;
		std::stop_source m_animStopSource{std::nostopstate};
		coro::SharedTask<void> m_animTask;
		float m_shimmerPosition{0.f};
	};
}
