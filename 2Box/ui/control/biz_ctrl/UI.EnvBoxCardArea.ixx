export module UI.EnvBoxCardArea;

import std;
import UI.Core;
import UI.EnvBoxCard;
import UI.ScrollBar;
import Biz.Core;
import Coroutine;

namespace ui
{
	export class EnvBoxCardArea final : public ControlBase
	{
	public:
		static constexpr float shadowSize = 6.f;
		static constexpr float shadowOffsetY = 4.f;

		template <typename... Args>
		explicit EnvBoxCardArea(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
		{
			initialize();
		}

		virtual ~EnvBoxCardArea();

		bool isNoEnvs() const noexcept { return m_envs.empty(); }
		void launchProcess(const std::wstring& procFullPath);

	protected:
		virtual void onResize(float width, float height) override;
		virtual void onMouseWheel(const MouseWheelEvent& e) override;

	private:
		void initialize();
		coro::LazyTask<void> onEnvCountChange(biz::EnvManager::EChangeType changeType, std::shared_ptr<biz::Env> env);

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		coro::AsyncScope m_asyncScope;

	private:
		std::map<std::uint32_t, std::unique_ptr<EnvBoxCard>> m_envs;
		std::unique_ptr<ScrollBar> m_scrollBar;
	};
}
