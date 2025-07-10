// ReSharper disable CppMemberFunctionMayBeStatic
export module StateMachine;

import std;

namespace sm
{
	template <typename StateIndexType>
	concept state_index_type = requires
	{
		requires std::is_enum_v<StateIndexType>;
		requires std::is_convertible_v<std::underlying_type_t<StateIndexType>, size_t>;
		requires std::is_unsigned_v<std::underlying_type_t<StateIndexType>>;
		StateIndexType::TotalCount;
		requires static_cast<std::underlying_type_t<StateIndexType>>(StateIndexType::TotalCount) > 0;
	};

	template <typename CtxType>
	concept ctx_type = !std::is_pointer_v<CtxType>;

	export
	template <state_index_type StateIndexType>
	struct TNextState
	{
		StateIndexType NextStateIndex = static_cast<StateIndexType>(0);
		bool bIsTransferImmediately = false;
	};

	template <typename StateType, typename StateIndexType, typename CtxRefType>
	concept state_type_for = requires(StateType& StateCtx, CtxRefType Ctx)
	{
		{ StateCtx.OnEnter(Ctx) } -> std::same_as<void>;
		{ StateCtx.OnUpdate(Ctx) } -> std::same_as<TNextState<StateIndexType>>;
		{ StateCtx.OnExit(Ctx) } -> std::same_as<void>;
	};

	struct StateCoroVisitor
	{
		template <typename State>
		std::coroutine_handle<> operator()(const State& InState) noexcept
		{
			return InState.Coro;
		}
	};

	export
	template <template<typename IndexType, IndexType> typename StateType, state_index_type StateIndexType, ctx_type CtxType>
	struct StateMachine
	{
		template <StateIndexType StateIndex>
		using TStateType = StateType<StateIndexType, StateIndex>;

		using UnderlyingIndexType = std::underlying_type_t<StateIndexType>;

		static_assert(std::is_convertible_v<UnderlyingIndexType, size_t>, "UnderlyingIndexType need to be convertible to size_t");
		static_assert(std::is_unsigned_v<UnderlyingIndexType>, "UnderlyingIndexType need to be unsigned");

		static constexpr UnderlyingIndexType TotalStateCount = static_cast<UnderlyingIndexType>(StateIndexType::TotalCount);

		static_assert(!std::is_pointer_v<CtxType>, "CtxType can't be a pointer");

		using CtxRefType = std::conditional_t<std::is_reference_v<CtxType>, CtxType, CtxType&>;
		using CtxPtrType = std::add_pointer_t<CtxRefType>;

	public:
		template <StateIndexType StateIndex>
		TStateType<StateIndex>& stateCtx()
		{
			using promise_type = typename TState<static_cast<UnderlyingIndexType>(StateIndex)>::promise_type;
			const auto& StateVariant = StateArray[static_cast<UnderlyingIndexType>(StateIndex)];
			return *std::coroutine_handle<promise_type>::from_address(std::visit(StateCoroVisitor{}, StateVariant).address()).promise().StateCtxPtr;
		}

		template <StateIndexType StateIndex>
		const TStateType<StateIndex>& stateCtx() const
		{
			using promise_type = typename TState<static_cast<UnderlyingIndexType>(StateIndex)>::promise_type;
			const auto& StateVariant = StateArray[static_cast<UnderlyingIndexType>(StateIndex)];
			return *std::coroutine_handle<promise_type>::from_address(std::visit(StateCoroVisitor{}, StateVariant).address()).promise().StateCtxPtr;
		}

		template <StateIndexType StateIndex>
		void transferTo()
		{
			if (CurrentStateIndex != static_cast<UnderlyingIndexType>(StateIndex))
			{
				ExitCurrentStateIfNeeded();
				CurrentStateIndex = static_cast<UnderlyingIndexType>(StateIndex);
			}
		}

		StateIndexType currentStateIndex() const
		{
			return static_cast<StateIndexType>(CurrentStateIndex);
		}

		StateIndexType stateIndexLastUpdated() const
		{
			return static_cast<StateIndexType>(StateIndexLastUpdated);
		}

		void setCtx(CtxPtrType InPtr)
		{
			CtxWrapper.CtxPtr = InPtr;
		}

		CtxRefType ctx() const
		{
			return *CtxWrapper.CtxPtr;
		}

		void update() const
		{
			const auto& StateVariant = StateArray[CurrentStateIndex];
			std::visit(StateCoroVisitor{}, StateVariant).resume();
		}

	public:
		StateMachine()
			: StateArray{GetStateArray(this, std::make_integer_sequence<UnderlyingIndexType, TotalStateCount>{})}
		{
		}

		StateMachine(const StateMachine&) = delete;
		StateMachine(StateMachine&&) noexcept = delete;
		StateMachine& operator=(const StateMachine&) = delete;
		StateMachine& operator=(StateMachine&&) noexcept = delete;

	private:
		template <UnderlyingIndexType StateIndex>
		struct TState
		{
		public:
			struct promise_type
			{
				TStateType<static_cast<StateIndexType>(StateIndex)>* StateCtxPtr;

				std::suspend_never initial_suspend() noexcept
				{
					return {};
				}

				std::suspend_always final_suspend() noexcept
				{
					return {};
				}

				void return_void() noexcept
				{
				}

				void unhandled_exception() noexcept
				{
				}

				TState get_return_object() noexcept
				{
					return TState{std::coroutine_handle<promise_type>::from_promise(*this)};
				}

				static consteval UnderlyingIndexType GetStateIndex() noexcept
				{
					return StateIndex;
				}
			};

			explicit TState(std::coroutine_handle<promise_type> InCoro) noexcept : Coro(InCoro)
			{
			}

			TState(TState&& That) noexcept : Coro(std::exchange(That.Coro, {}))
			{
			}

			~TState()
			{
				if (Coro)
				{
					Coro.destroy();
				}
			}

		public:
			std::coroutine_handle<promise_type> Coro = nullptr;
		};

		template <UnderlyingIndexType StateIndex>
		struct TAwaiter
		{
			StateMachine* Machine;
			TStateType<static_cast<StateIndexType>(StateIndex)>* StateCtxPtr;
			TNextState<StateIndexType> NextState;

			bool await_ready() noexcept
			{
				return false;
			}

			template <class Promise>
			std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> Coro) noexcept
			{
				Coro.promise().StateCtxPtr = StateCtxPtr;

				UnderlyingIndexType NextStateIndex = static_cast<UnderlyingIndexType>(NextState.NextStateIndex);
				if (StateIndex == NextStateIndex)
				{
					Machine->CurrentStateIndex = StateIndex;
					return std::noop_coroutine();
				}

				if (!NextState.bIsTransferImmediately)
				{
					Machine->CurrentStateIndex = NextStateIndex;
					return std::noop_coroutine();
				}

				const auto& StateVariant = Machine->StateArray[NextStateIndex];
				return std::visit(StateCoroVisitor{}, StateVariant);
			}

			void await_resume() const
			{
			}
		};

		template <UnderlyingIndexType StateIndex>
		TState<StateIndex> StateCoroutine()
		{
			state_type_for<StateIndexType, CtxRefType> auto StateCtx = TStateType<static_cast<StateIndexType>(StateIndex)>();
			TNextState<StateIndexType> NextState{static_cast<StateIndexType>(StateIndex)};
			bool bHasTransferred = true;

			while (true)
			{
				StateIndexLastUpdated = StateIndex;
				co_await TAwaiter<StateIndex>{this, &StateCtx, NextState};

				if (CtxWrapper.bIsDoInnerBiz)
				{
					if (!bHasTransferred)
					{
						bHasTransferred = true;
						if (CtxWrapper.CtxPtr)
						{
							StateCtx.OnExit(*CtxWrapper.CtxPtr);
						}
					}
					continue;
				}

				if (!CtxWrapper.CtxPtr)
				{
					continue;
				}

				CtxRefType Ctx = *CtxWrapper.CtxPtr;

				if (bHasTransferred)
				{
					StateCtx.OnEnter(Ctx);
				}

				NextState = StateCtx.OnUpdate(Ctx);

				bHasTransferred = NextState.NextStateIndex != static_cast<StateIndexType>(StateIndex);
				if (bHasTransferred)
				{
					StateCtx.OnExit(Ctx);
				}
			}
		}

	private:
		template <UnderlyingIndexType... Is>
		static constexpr auto GetStateArray(StateMachine* InMachinePtr, std::integer_sequence<UnderlyingIndexType, Is...>) -> std::array<std::variant<TState<Is>...>, sizeof...(Is)>
		{
			return std::array<std::variant<TState<Is>...>, sizeof...(Is)>{
				InMachinePtr->StateCoroutine<Is>()...
			};
		}

		using StateArrayType = decltype(GetStateArray(nullptr, std::make_integer_sequence<UnderlyingIndexType, TotalStateCount>{}));

		struct FCtxWrapper
		{
			CtxPtrType CtxPtr = nullptr;
			bool bIsDoInnerBiz = false;
		};

		struct FDoInnerBizGuard
		{
			explicit FDoInnerBizGuard(FCtxWrapper& InCtxWrapper)
				: CtxWrapper(InCtxWrapper)
			{
				CtxWrapper.bIsDoInnerBiz = true;
			}

			~FDoInnerBizGuard()
			{
				CtxWrapper.bIsDoInnerBiz = false;
			}

		private:
			FCtxWrapper& CtxWrapper;
		};

		void ExitCurrentStateIfNeeded()
		{
			FDoInnerBizGuard Guard(CtxWrapper);
			update();
		}

	private:
		FCtxWrapper CtxWrapper;
		UnderlyingIndexType CurrentStateIndex = 0;
		UnderlyingIndexType StateIndexLastUpdated = 0;
		StateArrayType StateArray;
	};
}
