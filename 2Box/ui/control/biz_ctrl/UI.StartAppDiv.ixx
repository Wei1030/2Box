export module UI.StartAppDiv;

import "sys_defs.h";
import std;
import UI.Core;
import UI.Button;

namespace ui
{
	export class StartAppDiv final : public ControlBase
	{
	public:
		template <typename... Args>
		explicit StartAppDiv(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
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
		void onBtnStartPressed();

	private:
		UniqueComPtr<IDWriteTextLayout> m_pExePathTextLayout;
		std::wstring m_strExePath;
		float m_pathTextHeight{0};
		std::unique_ptr<Button> m_btnStart;
	};
}
