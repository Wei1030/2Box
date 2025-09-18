// ReSharper disable CppMemberFunctionMayBeStatic
export module Coroutine:WhenAllSettled;

import std;
import :Concept;
import :PromiseBase;
import :Cancellation;
import :LazyTask;

namespace coro
{
	struct WhenAllSettledCounter
	{
		explicit WhenAllSettledCounter(std::size_t count)
			: counter(count)
		{
		}

		void startWait(std::coroutine_handle<> next)
		{
			cont = next;
		}

		std::coroutine_handle<> finishAndReturnNextIfNeed() noexcept
		{
			if (counter.fetch_sub(1, std::memory_order_acq_rel) == 1)
			{
				return cont;
			}
			return std::noop_coroutine();
		}

		std::atomic<std::size_t> counter;

		std::coroutine_handle<> cont{};
	};

	template <typename T>
	class WhenAllSettledPerTaskPromise : public PromiseTmplBase<T>
	{
	public:
		std::suspend_always initial_suspend() noexcept
		{
			return {};
		}

		auto final_suspend() noexcept
		{
			struct FinalAwaiter
			{
				WhenAllSettledPerTaskPromise& self;

				bool await_ready() const noexcept { return false; }

				std::coroutine_handle<> await_suspend(std::coroutine_handle<>) const noexcept
				{
					return self.m_counter->finishAndReturnNextIfNeed();
				}

				void await_resume() const noexcept
				{
				}
			};
			return FinalAwaiter{*this};
		}

		const std::stop_token& getCancellationToken() const noexcept
		{
			return m_token;
		}

		void setCancellationToken(std::stop_token token) noexcept
		{
			m_token = std::move(token);
		}

		void setCounter(WhenAllSettledCounter& counter)
		{
			m_counter = &counter;
		}

	private:
		std::stop_token m_token;
		WhenAllSettledCounter* m_counter{nullptr};
	};

	template <typename T>
	class WhenAllSettledPerTask
	{
	public:
		using ValueType = std::conditional_t<std::is_reference_v<T>, std::add_pointer_t<T>, T>;
		using ResultType = std::expected<ValueType, std::string>;

		struct promise_type final : public WhenAllSettledPerTaskPromise<ResultType>
		{
			WhenAllSettledPerTask get_return_object() noexcept
			{
				return WhenAllSettledPerTask{std::coroutine_handle<promise_type>::from_promise(*this)};
			}
		};

		WhenAllSettledPerTask() = delete;
		WhenAllSettledPerTask(const WhenAllSettledPerTask&) = delete;

		WhenAllSettledPerTask(WhenAllSettledPerTask&& that) noexcept : m_coro(std::exchange(that.m_coro, {}))
		{
		}

		~WhenAllSettledPerTask()
		{
			if (m_coro)
			{
				if (!m_coro.done())
				{
					std::unreachable();
				}
				m_coro.destroy();
			}
		}

		WhenAllSettledPerTask& operator=(const WhenAllSettledPerTask&) = delete;
		WhenAllSettledPerTask& operator=(WhenAllSettledPerTask&& that) = delete;

		void start(WhenAllSettledCounter& counter)
		{
			m_coro.promise().setCounter(counter);
			m_coro.resume();
		}

		using ResultTransferT = std::add_rvalue_reference_t<ResultType>;

		ResultTransferT transferResult()
		{
			return static_cast<ResultTransferT>(m_coro.promise().getValue());
		}

	private:
		explicit WhenAllSettledPerTask(std::coroutine_handle<promise_type> coro) noexcept : m_coro(coro)
		{
		}

		std::coroutine_handle<promise_type> m_coro = nullptr;
	};

	template <awaitable T>
	auto when_all_settled_per_task_impl(T a, std::stop_token token)
		-> WhenAllSettledPerTask<typename AwaitableTraits<std::remove_reference_t<T>>::AwaitResultT>
	{
		using AwaitResultT = typename AwaitableTraits<std::remove_reference_t<T>>::AwaitResultT;

		using NonRefAwaitResultT = typename AwaitableTraits<std::remove_reference_t<T>>::NonRefAwaitResultT;
		using ExpectedT = std::expected<NonRefAwaitResultT, std::string>;

		co_await set_cancellation_token(std::move(token));

		try
		{
			if constexpr (std::is_void_v<AwaitResultT>)
			{
				co_await a;
				co_return ExpectedT{};
			}
			else
			{
				std::add_lvalue_reference_t<AwaitResultT> result = co_await a;
				if constexpr (std::is_reference_v<AwaitResultT>)
				{
					co_return ExpectedT{&result};
				}
				else
				{
					co_return ExpectedT{std::move(result)};
				}
			}
		}
		catch (const std::exception& e)
		{
			co_return ExpectedT{std::unexpect_t{}, e.what()};
		}
		catch (...)
		{
			co_return ExpectedT{std::unexpect_t{}, "unknown error"};
		}
	}

	template <typename... Tasks>
	class WhenAllSettledHelper
	{
	public:
		explicit WhenAllSettledHelper(Tasks&&... tasks)
			noexcept(std::conjunction_v<std::is_nothrow_move_constructible<Tasks>...>)
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(tasks)...)
		{
		}

		explicit WhenAllSettledHelper(std::tuple<Tasks...>&& tasks)
			noexcept(std::is_nothrow_move_constructible_v<std::tuple<Tasks...>>)
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(tasks))
		{
		}

		WhenAllSettledHelper(WhenAllSettledHelper&& other) noexcept
			: m_counter(sizeof...(Tasks))
			  , m_tasks(std::move(other.m_tasks))
		{
		}

		WhenAllSettledCounter& getCounter() noexcept
		{
			return m_counter;
		}

		std::tuple<Tasks...>& getTasks() noexcept
		{
			return m_tasks;
		}

		auto waitAllDone() noexcept
		{
			struct Awaiter
			{
				WhenAllSettledHelper& self;
				bool await_ready() const noexcept { return false; }

				void await_suspend(std::coroutine_handle<> cont) noexcept
				{
					self.getCounter().startWait(cont);
					self.startTasks(std::make_integer_sequence<std::size_t, sizeof...(Tasks)>{});
				}

				void await_resume() noexcept
				{
				}
			};
			return Awaiter{*this};
		}

	private:
		template <std::size_t... Is>
		void startTasks(std::integer_sequence<std::size_t, Is...>) noexcept
		{
			(std::get<Is>(m_tasks).start(m_counter), ...);
		}

	private:
		WhenAllSettledCounter m_counter;
		std::tuple<Tasks...> m_tasks;
	};

	template <std::size_t... Is, awaitable... Ts>
	auto when_all_settled_impl(std::index_sequence<Is...>, Ts... awaitables)
		-> LazyTask<std::tuple<std::expected<typename AwaitableTraits<std::remove_reference_t<Ts>>::NonRefAwaitResultT, std::string>...>>
	{
		std::stop_token cancelToken = co_await get_current_cancellation_token();
		
		WhenAllSettledHelper whenAllSettledHelper{
			when_all_settled_per_task_impl(std::move(awaitables), cancelToken)
			...
		};

		co_await whenAllSettledHelper.waitAllDone();

		auto& tasks = whenAllSettledHelper.getTasks();
		co_return std::tuple<std::expected<typename AwaitableTraits<std::remove_reference_t<Ts>>::NonRefAwaitResultT, std::string>...>{
			std::get<Is>(tasks).transferResult()
			...
		};
	}

	export
	template <awaitable... Ts>
		requires (sizeof...(Ts) > 0)
	auto when_all_settled(Ts... awaitables)
	{
		return when_all_settled_impl(std::make_index_sequence<sizeof...(Ts)>{}, std::move(awaitables)...);
	}
}
