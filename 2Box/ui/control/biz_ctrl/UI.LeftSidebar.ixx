export module UI.LeftSidebar;

import UI.Core;

namespace ui
{
	export class LeftSidebar final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;
	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;
	};
}
