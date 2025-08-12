export module UI.StartAppDiv;

import UI.Core;

namespace ui
{
	export class StartAppDiv final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;
	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;
	};
}
