export module UI.LeftSidebar;

import std;
import UI.Core;
import UI.StartAppDiv;
import UI.EnvBoxCard;

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

	private:
		void initialize();
		void initializeAllEnvBoxCard();

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		std::unique_ptr<StartAppDiv> m_startAppDiv;
		std::vector<std::unique_ptr<EnvBoxCard>> m_envs;
	};
}
