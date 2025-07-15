export module UI.MainPage:Home;

import "sys_defs.h";
import :Define;
import StateMachine;
import UI.PageBase;

namespace ui
{
	export
	template <>
	class TMainPageType<MainPageType, MainPageType::Home> final : public PageBase
	{
	public:
		void OnEnter(WindowBase&)
		{
		}

		sm::TNextState<MainPageType> OnUpdate(WindowBase&)
		{
			return {MainPageType::Home};
		}

		void OnExit(WindowBase&)
		{
		}

		virtual void draw(const RenderContext& renderCtx) override
		{
			renderCtx.renderTarget->Clear(D2D1::ColorF(0xFFFFFF));
		}
	};
}
