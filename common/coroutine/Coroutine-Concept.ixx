export module Coroutine:Concept;

import std;

namespace coro
{
	template <typename, template <typename...> typename>
	constexpr bool IS_SPECIALIZATION_OF = false;
	template <typename... Args, template <typename...> typename Templ>
	constexpr bool IS_SPECIALIZATION_OF<Templ<Args...>, Templ> = true;

	export
	template <typename T>
	concept await_suspend_result =
		std::same_as<T, void> || std::same_as<T, bool> || IS_SPECIALIZATION_OF<T, std::coroutine_handle>;

#ifdef __INTELLISENSE__
	using AnyCoroutineHandle = std::coroutine_handle<>;
#else
	struct AnyCoroutineHandle
	{
		template <typename Promise>
		operator std::coroutine_handle<Promise>() const noexcept
		{
			return {};
		}
	};
#endif

	export
	template <typename T>
	concept simple_awaitable = requires(T& val, AnyCoroutineHandle handle)
	{
		{ val.await_ready() } -> std::convertible_to<bool>;
		{ val.await_suspend(handle) } -> await_suspend_result;
		val.await_resume();
	};

	export
	template <typename T>
	concept has_member_co_await = requires(T&& val)
	{
		{ static_cast<T&&>(val).operator co_await() } -> simple_awaitable;
	};

	export
	template <typename T>
	concept has_adl_co_await = requires(T&& val)
	{
		{ operator co_await(static_cast<T&&>(val)) } -> simple_awaitable;
	};

	export
	template <typename T>
	concept awaitable = has_member_co_await<T> || has_adl_co_await<T> || simple_awaitable<T>;

	template <simple_awaitable T>
	decltype(auto) get_awaiter(T&& t)
	{
		return std::forward<T>(t);
	}

	template <has_member_co_await T>
	decltype(auto) get_awaiter(T&& t)
	{
		return std::forward<T>(t).operator co_await();
	}

	template <has_adl_co_await T>
	decltype(auto) get_awaiter(T&& t)
	{
		return operator co_await(std::forward<T>(t));
	}

	export
	template <typename T = void>
	struct NonVoidAwaitResultType;

	template <typename T>
	struct NonVoidAwaitResultType
	{
		using Type = T;
	};

	template <>
	struct NonVoidAwaitResultType<void>
	{
		using Type = NonVoidAwaitResultType;
	};

	export
	template <awaitable T>
	struct AwaitableTraits
	{
		using AwaiterT = decltype(get_awaiter<T>(std::declval<T>()));
		using AwaitResultT = decltype(get_awaiter<T>(std::declval<T>()).await_resume());
		using NonVoidAwaitResultT = typename NonVoidAwaitResultType<AwaitResultT>::Type;

		using ResultTransferT = std::conditional_t<
			std::is_reference_v<AwaitResultT>,
			AwaitResultT, // 保持引用类型不变
			std::add_rvalue_reference_t<AwaitResultT> // 值类型转右值引用
		>;
	};
}
