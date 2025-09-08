export module UI.EnvDetail;

import "sys_defs.h";
import std;
import UI.Core;
import UI.Button;
import UI.ScrollBar;
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
		const std::shared_ptr<biz::Env>& getEnv() const noexcept { return m_env; }
		void setEnv(const std::shared_ptr<biz::Env>& env);
		void clearEnv();
		void procCountChange(biz::Env::EProcEvent e, const std::shared_ptr<biz::ProcessInfo>& proc);
		bool hasAnyProcesses() const noexcept { return m_processes.size() > 0; }

	protected:
		virtual void onResize(float width, float height) override;
		virtual void onMouseEnter(const MouseEvent& e) override;
		virtual void onMouseLeave(const MouseEvent& e) override;
		virtual void onMouseWheel(const MouseWheelEvent& e) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		void updateAllItemPos();

	private:
		ScrollBar m_scrollBar{this};
		bool m_isHovered = false;

	private:
		std::shared_ptr<biz::Env> m_env;

		struct ListItem
		{
			std::shared_ptr<biz::ProcessInfo> process;
			D2D1_RECT_F rect;
		};

		std::map<std::uint32_t, ListItem> m_processes;
		std::vector<ListItem*> m_processesToDraw;
	};

	export class EnvDetail final : public ControlBase
	{
	public:
		template <typename... Args>
		explicit EnvDetail(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
		{
			initialize();
		}

	private:
		void initialize();

	public:
		ProcessList& getProcessList() { return m_processList; }
		bool hasDetail() const noexcept { return m_processList.hasEnv(); }

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
		UniqueComPtr<IDWriteTextLayout> m_noProcTextLayout;
		float m_noProcTextHeight{0};
		Button m_btnClear{this};
		Button m_btnLaunch{this};
		ProcessList m_processList{this};
	};
}
