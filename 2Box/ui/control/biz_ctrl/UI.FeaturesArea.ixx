export module UI.FeaturesArea;

import std;
import UI.Core;

namespace ui
{
	export class FeaturesArea final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;
	};
}
