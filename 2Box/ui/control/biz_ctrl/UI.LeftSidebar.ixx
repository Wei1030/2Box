export module UI.LeftSidebar;

import std;
import UI.Core;
import UI.StartAppDiv;
import UI.EnvBoxCardArea;

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

		EnvBoxCardArea* getEnvBoxCardArea() const noexcept { return m_envCardsArea.get(); }

	protected:
		virtual void onResize(float width, float height) override;

	private:
		void initialize();
		void resizeEnvCardsArea() const;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		std::unique_ptr<StartAppDiv> m_startAppDiv;
		std::unique_ptr<EnvBoxCardArea> m_envCardsArea;
	};
}
