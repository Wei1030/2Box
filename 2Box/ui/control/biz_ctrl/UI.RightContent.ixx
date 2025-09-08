export module UI.RightContent;

import std;
import UI.Core;
import UI.FeaturesArea;
export import UI.EnvDetail;
import Biz.Core;

namespace ui
{
	export class RightContent final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

	public:
		EnvDetail& getEnvDetail() { return m_envDetail; }
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
