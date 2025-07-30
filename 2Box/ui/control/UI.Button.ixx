export module UI.Button;

import "sys_defs.h";
import std;
import UI.Core;
import StateMachine;

namespace ui
{
	export class Button;

	namespace button_detail
	{
		enum class EPainterType : std::uint8_t
		{
			Normal,
			Hover,
			Active,
			Disabled,
			TotalCount
		};

		template <typename PainterEnumType, PainterEnumType>
		class TPainterType
		{
		};

		struct PainterContext
		{
			Button& btn;
			const RenderContext& renderCtx;
		};

		// ReSharper disable CppMemberFunctionMayBeStatic
		// ReSharper disable CppInconsistentNaming

		class PainterBase
		{
		public:
			void OnEnter(PainterContext&)
			{
			}

			void OnExit(PainterContext&)
			{
			}
		};

		template <>
		class TPainterType<EPainterType, EPainterType::Normal> : public PainterBase
		{
		public:
			inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx) const;
		};

		template <>
		class TPainterType<EPainterType, EPainterType::Hover> : public PainterBase
		{
		public:
			inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx) const;
		};

		template <>
		class TPainterType<EPainterType, EPainterType::Active> : public PainterBase
		{
		public:
			inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx) const;
		};

		template <>
		class TPainterType<EPainterType, EPainterType::Disabled> : public PainterBase
		{
		public:
			inline sm::TNextState<EPainterType> OnUpdate(PainterContext& ctx) const;
		};

		// ReSharper restore CppParameterMayBeConstPtrOrRef
		// ReSharper restore CppInconsistentNaming
	}

	class Button : public ControlBase
	{
		using EPainterType = button_detail::EPainterType;
		using PainterContext = button_detail::PainterContext;
		using PainterNormalStateCtx = button_detail::TPainterType<EPainterType, EPainterType::Normal>;
		using PainterHoverStateCtx = button_detail::TPainterType<EPainterType, EPainterType::Hover>;
		using PainterActiveStateCtx = button_detail::TPainterType<EPainterType, EPainterType::Active>;
		using PainterDisabledStateCtx = button_detail::TPainterType<EPainterType, EPainterType::Disabled>;

	public:
		template <typename... Args>
		explicit Button(Args&&... args) noexcept : ControlBase(std::forward<Args>(args)...)
		{
			initialize();
		}

	protected:
		virtual void onMouseEnter(const MouseEvent& e) override;
		virtual void onMouseLeave(const MouseEvent& e) override;
		virtual void onMouseDown(const MouseEvent& e) override;
		virtual void onMouseUp(const MouseEvent& e) override;
		virtual void onClick(const MouseEvent& e) override;

	private:
		void initialize();
		virtual void drawImpl(const RenderContext& renderCtx) override;

		template <typename PainterEnumType, PainterEnumType>
		friend class button_detail::TPainterType;


		void drawNormal(const RenderContext& renderCtx, const PainterNormalStateCtx& stateCtx) const;
		void drawHover(const RenderContext& renderCtx, const PainterHoverStateCtx& stateCtx) const;
		void drawActive(const RenderContext& renderCtx, const PainterActiveStateCtx& stateCtx) const;
		void drawDisabled(const RenderContext& renderCtx, const PainterDisabledStateCtx& stateCtx) const;

	private:
		sm::StateMachine<button_detail::TPainterType, EPainterType, PainterContext> m_painter;
		bool m_bIsHover{false};
	};


	namespace button_detail
	{
		// ReSharper disable CppMemberFunctionMayBeStatic
		// ReSharper disable CppParameterMayBeConstPtrOrRef

		sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Normal>::OnUpdate(PainterContext& ctx) const
		{
			ctx.btn.drawNormal(ctx.renderCtx, *this);
			return {EPainterType::Normal};
		}

		sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Hover>::OnUpdate(PainterContext& ctx) const
		{
			ctx.btn.drawHover(ctx.renderCtx, *this);
			return {EPainterType::Hover};
		}

		sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Active>::OnUpdate(PainterContext& ctx) const
		{
			ctx.btn.drawActive(ctx.renderCtx, *this);
			return {EPainterType::Active};
		}

		sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Disabled>::OnUpdate(PainterContext& ctx) const
		{
			ctx.btn.drawDisabled(ctx.renderCtx, *this);
			return {EPainterType::Disabled};
		}

		// ReSharper restore CppParameterMayBeConstPtrOrRef
		// ReSharper restore CppMemberFunctionMayBeStatic
	}
}
