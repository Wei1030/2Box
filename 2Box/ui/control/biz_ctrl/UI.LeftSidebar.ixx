export module UI.LeftSidebar;

import std;
import UI.Core;
import UI.StartAppDiv;
import UI.EnvBoxCard;
import Biz.Core;
import Coroutine;

namespace ui
{
	export class LeftSidebar final : public ControlBase
	{
	public:
		template <typename... Args>
		explicit LeftSidebar(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
		{
			initialize();
		}

		virtual ~LeftSidebar();

		std::wstring selectProcess() const;
		void launchProcess(std::wstring_view procFullPath);
		
	private:
		void initialize();
		void initializeAllEnvBoxCard();
		coro::LazyTask<void> onEnvCountChange(biz::EnvManager::EChangeType changeType, std::shared_ptr<biz::Env> env);

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		coro::AsyncScope m_asyncScope;

	private:
		std::unique_ptr<StartAppDiv> m_startAppDiv;
		std::map<std::uint32_t, std::unique_ptr<EnvBoxCard>> m_envs;
	};
}
