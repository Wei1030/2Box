export module Coroutine:Cancellation;

import std;
import :Concept;
import :PromiseBase;
import :LazyTask;

namespace coro
{
	export struct CancellationAwaiter
	{
		std::stop_token token{};

		constexpr bool await_ready() const noexcept
		{
			return false;
		}

		template <typename T>
			requires requires(std::coroutine_handle<T> h)
			{
				h.promise().getCancellationToken();
			}
		bool await_suspend(std::coroutine_handle<T> h) noexcept
		{
			token = h.promise().getCancellationToken();
			return false;
		}

		std::stop_token& await_resume() noexcept
		{
			return token;
		}
	};

	export LazyTask<std::stop_token> get_current_cancellation_token()
	{
		co_return co_await CancellationAwaiter{};
	}

	export struct CancellationSetter
	{
		std::stop_token token;

		constexpr bool await_ready() const noexcept
		{
			return false;
		}

		template <typename T>
			requires requires(std::coroutine_handle<T> h)
			{
				h.promise().setCancellationToken(std::declval<std::stop_token>());
			}
		bool await_suspend(std::coroutine_handle<T> h) noexcept
		{
			h.promise().setCancellationToken(token);
			return false;
		}

		constexpr void await_resume() const noexcept
		{
		}
	};

	export
	[[nodiscard]] CancellationSetter set_cancellation_token(std::stop_token token) noexcept
	{
		return {.token = std::move(token)};
	}

	export
	template <awaitable T>
	auto co_with_cancellation(T a, std::stop_token token)
		-> LazyTask<typename AwaitableTraits<std::remove_reference_t<T>>::AwaitResultT>
	{
		co_await CancellationSetter{.token = std::move(token)};
		co_return co_await a;
	}
}
