export module UI.LeftSidebar;

import std;
import UI.Core;
import UI.StartAppDiv;

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

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		std::unique_ptr<StartAppDiv> m_startAppDiv;
	};
}
