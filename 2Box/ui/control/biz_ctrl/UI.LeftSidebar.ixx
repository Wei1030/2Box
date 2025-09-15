export module UI.LeftSidebar;

import std;
import UI.Core;
export import UI.StartAppDiv;
export import UI.EnvBoxCardArea;

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

		StartAppDiv* getStartAppDiv() const noexcept { return m_startAppDiv.get(); }
		EnvBoxCardArea* getEnvBoxCardArea() const noexcept { return m_envCardsArea.get(); }

	protected:
		virtual void onResize(float width, float height) override;

	private:
		void initialize();

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		std::unique_ptr<StartAppDiv> m_startAppDiv;
		std::unique_ptr<EnvBoxCardArea> m_envCardsArea;
	};
}
