export module UI.RightContent;

import std;
import UI.Core;
import UI.FeaturesArea;
export import UI.ProcessList;
import Biz.Core;

namespace ui
{
	export class RightContent final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

	public:
		ProcessList& getProcessList() { return m_envDetail.getProcessList(); }

	protected:
		virtual void onResize(float width, float height) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		FeaturesArea m_featuresArea{this};
		EnvDetail m_envDetail{this};
	};
}
