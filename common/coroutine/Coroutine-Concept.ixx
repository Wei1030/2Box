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

	export
	template <typename T, typename Promise = void>
	concept simple_awaitable = requires(T& val, const std::coroutine_handle<Promise>& coro)
	{
		{ val.await_ready() } -> std::convertible_to<bool>;
		{ val.await_suspend(coro) } -> await_suspend_result;
		val.await_resume();
	};

	export
	template <typename T, typename Promise = void>
	concept has_member_co_await = requires(T&& val)
	{
		{ static_cast<T&&>(val).operator co_await() } -> simple_awaitable<Promise>;
	};

	export
	template <typename T, typename Promise = void>
	concept has_adl_co_await = requires(T&& val)
	{
		{ operator co_await(static_cast<T&&>(val)) } -> simple_awaitable<Promise>;
	};

	export
	template <typename T, typename Promise = void>
	concept awaitable = has_member_co_await<T, Promise> || has_adl_co_await<T, Promise> || simple_awaitable<T&, Promise>;


	template <typename T, typename Promise = void>
		requires simple_awaitable<T, Promise>
	decltype(auto) get_awaiter(T&& t)
	{
		return std::forward<T>(t);
	}

	template <typename T, typename Promise = void>
		requires has_member_co_await<T, Promise>
	decltype(auto) get_awaiter(T&& t)
	{
		return std::forward<T>(t).operator co_await();
	}

	template <typename T, typename Promise = void>
		requires has_adl_co_await<T, Promise>
	decltype(auto) get_awaiter(T&& t)
	{
		return operator co_await(std::forward<T>(t));
	}

	export
	template <typename T, typename Promise = void>
		requires awaitable<T, Promise>
	struct AwaitableTraits
	{
		using AwaiterT = decltype(get_awaiter<T, Promise>(std::declval<T>()));
		using AwaitResultT = decltype(get_awaiter<T, Promise>(std::declval<T>()).await_resume());
	};
}
