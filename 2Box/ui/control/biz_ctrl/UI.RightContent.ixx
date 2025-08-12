export module UI.RightContent;

import UI.Core;

namespace ui
{
	export class RightContent final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;
	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;
	};
}
