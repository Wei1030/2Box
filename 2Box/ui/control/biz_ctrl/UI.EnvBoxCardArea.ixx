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
		static constexpr float scrollWidth = 8.f;
		static constexpr float scrollAreaWidth = scrollWidth + 8.f;

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
		virtual void onMouseEnter(const MouseEvent& e) override;
		virtual void onMouseLeave(const MouseEvent& e) override;
		virtual void onMouseWheel(const MouseWheelEvent& e) override;

	private:
		void initialize();
		coro::LazyTask<void> onEnvCountChange(biz::EnvManager::EChangeType changeType, std::shared_ptr<biz::Env> env);
		void onEnvSelected(EnvBoxCard* selected);
		void updateAllEnvPos();

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		coro::AsyncScope m_asyncScope;

	private:
		std::map<std::uint32_t, std::unique_ptr<EnvBoxCard>> m_envs;
		std::vector<EnvBoxCard*> m_envsToDraw;
		EnvBoxCard* m_currentSelectedEnv{nullptr};
		std::unique_ptr<ScrollBar> m_scrollBar;
		bool m_isHovered = false;
	};
}
