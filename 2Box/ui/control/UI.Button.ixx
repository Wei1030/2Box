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

		public:
			D2D1_COLOR_F textColor{D2D1::ColorF(0xffffff)};
			D2D1_COLOR_F backgroundColor{D2D1::ColorF(0x000000)};
			D2D1_COLOR_F borderColor{D2D1::ColorF(0x000000, 0.f)};
			IDWriteTextFormat* textFormat{nullptr};
			UniqueComPtr<IDWriteTextLayout> textLayout;
			std::wstring text;
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

	export class Button : public ControlBase
	{
		using EPainterType = button_detail::EPainterType;
		using PainterContext = button_detail::PainterContext;
		using PainterStateCtx = button_detail::PainterBase;
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

		enum class EState
		{
			Normal,
			Hover,
			Active,
			Disabled,
			All
		};

		void setTextColor(const D2D1_COLOR_F& color, EState state = EState::All)
		{
			if (state == EState::All)
			{
				applyAllState(std::make_index_sequence<static_cast<std::size_t>(EState::All)>(), &Button::setTextColor, this, color);
				return;
			}
			ctxFromState(state).textColor = color;
		}

		void setBackgroundColor(const D2D1_COLOR_F& color, EState state = EState::All)
		{
			if (state == EState::All)
			{
				applyAllState(std::make_index_sequence<static_cast<std::size_t>(EState::All)>(), &Button::setBackgroundColor, this, color);
				return;
			}
			ctxFromState(state).backgroundColor = color;
		}

		void setBorderColor(const D2D1_COLOR_F& color, EState state = EState::All)
		{
			if (state == EState::All)
			{
				applyAllState(std::make_index_sequence<static_cast<std::size_t>(EState::All)>(), &Button::setBorderColor, this, color);
				return;
			}
			ctxFromState(state).borderColor = color;
		}

		void setTextFormat(IDWriteTextFormat* textFormat, EState state = EState::All);
		void setText(std::wstring_view text, EState state = EState::All);

		void setOnClick(std::move_only_function<void()> func)
		{
			m_onClick = std::move(func);
		}

		using DrawCallback = std::function<void(const RenderContext& renderCtx, EState state)>;
		void setDrawCallback(DrawCallback func) { m_drawCallback = std::move(func); }

		void setDontDrawDefault(bool b = true) { m_bDontDraw = b; }

		virtual void drawImpl(const RenderContext& renderCtx) override;

	protected:
		virtual void onMouseEnter(const MouseEvent& e) override;
		virtual void onMouseLeave(const MouseEvent& e) override;
		virtual void onMouseDown(const MouseEvent& e) override;
		virtual void onMouseUp(const MouseEvent& e) override;
		virtual void onClick(const MouseEvent& e) override;

	private:
		void initialize();

		template <typename PainterEnumType, PainterEnumType>
		friend class button_detail::TPainterType;

		void drawByState(const RenderContext& renderCtx, const PainterStateCtx& stateCtx) const;

	private:
		button_detail::PainterBase& ctxFromState(EState state)
		{
			switch (state)
			{
			case EState::Normal:
				return m_painter.stateCtx<EPainterType::Normal>();
			case EState::Hover:
				return m_painter.stateCtx<EPainterType::Hover>();
			case EState::Active:
				return m_painter.stateCtx<EPainterType::Active>();
			case EState::Disabled:
				return m_painter.stateCtx<EPainterType::Disabled>();
			default:
				return m_painter.stateCtx<EPainterType::Normal>();
			}
		}

		template <std::size_t... Is, typename Func, typename... Args>
		void applyAllState(std::index_sequence<Is...>, Func&& func, Args&&... args)
		{
			(std::invoke(std::forward<Func>(func), std::forward<Args>(args)..., static_cast<EState>(Is)), ...);
		}

	private:
		sm::StateMachine<button_detail::TPainterType, EPainterType, PainterContext> m_painter;
		std::move_only_function<void()> m_onClick;
		DrawCallback m_drawCallback;
		bool m_bIsHover{false};
		bool m_bDontDraw{false};
	};


	namespace button_detail
	{
		// ReSharper disable CppMemberFunctionMayBeStatic
		// ReSharper disable CppParameterMayBeConstPtrOrRef

		sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Normal>::OnUpdate(PainterContext& ctx) const
		{
			if (!ctx.btn.m_bDontDraw)
			{
				ctx.btn.drawByState(ctx.renderCtx, *this);
			}
			if (ctx.btn.m_drawCallback)
			{
				ctx.btn.m_drawCallback(ctx.renderCtx, Button::EState::Normal);
			}
			return {EPainterType::Normal};
		}

		sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Hover>::OnUpdate(PainterContext& ctx) const
		{
			if (!ctx.btn.m_bDontDraw)
			{
				ctx.btn.drawByState(ctx.renderCtx, *this);
			}
			if (ctx.btn.m_drawCallback)
			{
				ctx.btn.m_drawCallback(ctx.renderCtx, Button::EState::Hover);
			}
			return {EPainterType::Hover};
		}

		sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Active>::OnUpdate(PainterContext& ctx) const
		{
			if (!ctx.btn.m_bDontDraw)
			{
				ctx.btn.drawByState(ctx.renderCtx, *this);
			}
			if (ctx.btn.m_drawCallback)
			{
				ctx.btn.m_drawCallback(ctx.renderCtx, Button::EState::Active);
			}
			return {EPainterType::Active};
		}

		sm::TNextState<EPainterType> TPainterType<EPainterType, EPainterType::Disabled>::OnUpdate(PainterContext& ctx) const
		{
			if (!ctx.btn.m_bDontDraw)
			{
				ctx.btn.drawByState(ctx.renderCtx, *this);
			}
			if (ctx.btn.m_drawCallback)
			{
				ctx.btn.m_drawCallback(ctx.renderCtx, Button::EState::Disabled);
			}
			return {EPainterType::Disabled};
		}

		// ReSharper restore CppParameterMayBeConstPtrOrRef
		// ReSharper restore CppMemberFunctionMayBeStatic
	}
}
