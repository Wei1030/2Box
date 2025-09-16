export module UI.FeaturesArea;

import "sys_defs.h";
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
		void drawCheckBox(const RenderContext& renderCtx, Button::EState state) const;

	private:
		Button m_tileWndBtn{this};
		Button m_tileCheckbox{this};
		bool m_bMaintainAspectRatio{false};
		float m_checkBoxTextWidth;
		float m_checkBoxTextHeight;
		UniqueComPtr<IDWriteTextLayout> m_checkBoxTextLayout;
		Button m_syncBtn{this};
	};
}
