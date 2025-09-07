export module UI.RightContent;

import std;
import UI.Core;
import Biz.Core;

namespace ui
{
	export class RightContent final : public ControlBase
	{
	public:
		using ControlBase::ControlBase;

	public:
		void showEnvInfo(const std::shared_ptr<biz::Env>& env);
		void hideEnvInfo();

		void procCountChange(biz::Env::EProcEvent e, const std::shared_ptr<biz::ProcessInfo>& proc);
		
	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		std::shared_ptr<biz::Env> m_env;
		std::map<std::uint32_t, std::shared_ptr<biz::ProcessInfo>> m_processes;
	};
}
