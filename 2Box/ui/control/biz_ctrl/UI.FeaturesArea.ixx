export module UI.FeaturesArea;

import std;
import UI.Core;
import UI.Button;

namespace ui
{
	export class FeaturesArea final : public ControlBase
	{
	public:
		template <typename... Args>
		explicit FeaturesArea(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
		{
			initialize();
		}

	private:
		void initialize();

	protected:
		virtual void onResize(float width, float height) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		Button m_tileWndBtn{this};
	};
}
