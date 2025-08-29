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

	private:
		void initialize();

	protected:
		virtual void onResize(float width, float height) override;
		virtual void onMouseEnter(const MouseEvent& e) override;
		virtual void onMouseLeave(const MouseEvent& e) override;

	private:
		virtual void drawImpl(const RenderContext& renderCtx) override;

	private:
		coro::LazyTask<void> onProcessCountChange(std::size_t count);
		void onBtnStartPressed();

	private:
		std::unique_ptr<Button> m_btnStart;
		bool m_isHovered = false;
		bool m_isPressed = false;
		coro::AsyncScope m_asyncScope;

	private:
		std::shared_ptr<biz::Env> m_env;
		std::wstring m_name;
		std::wstring m_strProcCount{L"0"};
	};
}
