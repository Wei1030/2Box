export module UI.RightContent;

import "sys_defs.h";
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
		template <typename... Args>
		explicit RightContent(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
		{
			initialize();
		}

	private:
		void initialize();

	public:
		EnvDetail& getEnvDetail() { return m_envDetail; }
		ProcessList& getProcessList() { return m_envDetail.getProcessList(); }

	protected:
		virtual void onResize(float width, float height) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		UniqueComPtr<IDWriteTextFormat> m_importantNoticeFormat;
		UniqueComPtr<IDWriteTextLayout> m_mainTipsLayout;
		float m_mainTipsWidth{0};
		FeaturesArea m_featuresArea{this};
		EnvDetail m_envDetail{this};
	};
}
