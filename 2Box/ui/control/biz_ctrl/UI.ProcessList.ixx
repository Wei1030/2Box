export module UI.ProcessList;

import std;
import UI.Core;

namespace ui
{
	export class ProcessList final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;
	};
}
