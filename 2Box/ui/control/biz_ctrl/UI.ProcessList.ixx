export module UI.ProcessList;

import "sys_defs.h";
import std;
import UI.Core;
import UI.Button;
import Biz.Core;

namespace ui
{
	export class ProcessList final : public ControlBase
	{
	public:
		template <typename... Args>
		explicit ProcessList(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
		{
			initialize();
		}

	private:
		void initialize();

	public:
		bool hasEnv() const noexcept { return m_env != nullptr; }
		void setEnv(const std::shared_ptr<biz::Env>& env);
		void clearEnv();
		void procCountChange(biz::Env::EProcEvent e, const std::shared_ptr<biz::ProcessInfo>& proc);

	protected:
		virtual void onResize(float width, float height) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		void setProcPath(std::wstring_view path);
		void onLaunchBtnClick();

	private:
		UniqueComPtr<IDWriteTextLayout> m_procPathTextLayout;
		std::wstring m_strProcPath;
		float m_pathTextHeight{0};
		Button m_btnClear{this};
		Button m_btnLaunch{this};

	private:
		std::shared_ptr<biz::Env> m_env;
		std::map<std::uint32_t, std::shared_ptr<biz::ProcessInfo>> m_processes;
	};
}
