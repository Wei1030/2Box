export module UI.EnvBoxCard;

import "sys_defs.h";
import std;
import UI.Core;
import UI.Button;
import Coroutine;
import Env;

namespace ui
{
	export class EnvBoxCard final : public ControlBase
	{
	public:
		template <typename... Args>
		explicit EnvBoxCard(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
		{
			initialize();
		}

		virtual ~EnvBoxCard();

	public:
		void setEnv(const std::shared_ptr<biz::Env>& env);
		bool isHovered() const noexcept { return m_isHovered; }
		bool isIdle() const noexcept { return m_bIdle; }
		bool contains(const std::wstring& procFullPath) const { return m_env->contains(procFullPath); }
		void launchProcess(std::wstring_view procFullPath);

	private:
		void initialize();

	protected:
		virtual void onResize(float width, float height) override;
		virtual void onMouseEnter(const MouseEvent& e) override;
		virtual void onMouseLeave(const MouseEvent& e) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		coro::LazyTask<void> resetToIdleLater();
		coro::LazyTask<void> onProcessCountChange(biz::Env::EProcEvent e, std::shared_ptr<biz::ProcessInfo> proc, std::size_t count);
		void onBtnStartPressed();

	private:
		std::unique_ptr<Button> m_btnStart;
		std::unique_ptr<Button> m_btnDelete;
		bool m_isHovered = false;
		bool m_isPressed = false;
		coro::AsyncScope m_asyncScope;

	private:
		std::shared_ptr<biz::Env> m_env;
		std::wstring m_name;
		std::size_t m_procCount{0};
		std::wstring m_strProcCount{L"0"};
		bool m_bIdle{true};
		std::stop_source m_stopSource{std::nostopstate};
		// std::map<std::uint32_t, std::shared_ptr<biz::ProcessInfo>> m_processes;
	};
}
