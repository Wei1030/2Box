module UI.LoadingIndicator;

import MainApp;
import Scheduler;

namespace ui
{
	coro::LazyTask<void> LoadingIndicator::updateShimmerPosition()
	{
		static constexpr std::uint64_t intervalMs = 1000 / 60;
		static constexpr float desiredAnimDurationInMs = 2000.f;

		auto currentTimePoint = std::chrono::steady_clock::now();

		while (true)
		{
			co_await sched::transfer_after(std::chrono::milliseconds{intervalMs}, app().get_scheduler());
			// calc actual duration
			const auto lastTimePoint = currentTimePoint;
			currentTimePoint = std::chrono::steady_clock::now();
			const std::uint64_t actualIntervalMs = std::chrono::duration_cast<std::chrono::milliseconds>(currentTimePoint - lastTimePoint).count();

			// speed = total width / desired duration
			const float width = size().width;
			const float totalWidth = width * 2.f;
			const float speed = totalWidth / desiredAnimDurationInMs;

			m_shimmerPosition += speed * actualIntervalMs;
			if (m_shimmerPosition >= width || m_shimmerPosition < -width)
			{
				m_shimmerPosition = -width;
			}

			update();
		}
	}
}
