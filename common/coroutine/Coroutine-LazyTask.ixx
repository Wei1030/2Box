// ReSharper disable CppMemberFunctionMayBeStatic
export module Coroutine:LazyTask;

import :PromiseBase;
import std;

namespace coro
{
	export
	template <typename>
	class [[nodiscard]] LazyTask;

	template <typename T>
	class LazyTaskPromise : public PromiseTmplBase<T>
	{
	public:
		template <typename>
		friend class LazyTask;

		std::suspend_always initial_suspend() noexcept
		{
			return {};
		}

		struct FinalAwaiter
		{
			bool await_ready() noexcept
			{
				return false;
			}

			template <typename Promise>
			std::coroutine_handle<> await_suspend(std::coroutine_handle<Promise> coro) noexcept
			{
				auto& promise = coro.promise();
				return promise.m_cont ? promise.m_cont : std::noop_coroutine();
			}

			void await_resume() noexcept
			{
			}
		};

		FinalAwaiter final_suspend() noexcept
		{
			return {};
		}

		const std::stop_token& getCancellationToken() const noexcept
		{
			return m_token;
		}

		void setCancellationToken(std::stop_token token) noexcept
		{
			m_token = std::move(token);
		}

	private:
		std::coroutine_handle<> m_cont;
		std::stop_token m_token;
	};

	template <typename T>
	class [[nodiscard]] LazyTask
	{
	public:
		static_assert(std::is_void_v<T> || std::is_reference_v<T>
		              || (std::is_object_v<T> && std::is_move_constructible_v<T>),
		              "LazyTask's first template argument must be void, a reference type, or a "
		              "move-constructible object type");
		using value_type = T;

		struct promise_type final : public LazyTaskPromise<T>
		{
			LazyTask get_return_object() noexcept
			{
				return LazyTask{std::coroutine_handle<promise_type>::from_promise(*this)};
			}
		};

		LazyTask() = delete;

		LazyTask(const LazyTask&) = delete;

		LazyTask(LazyTask&& that) noexcept : m_coro(std::exchange(that.m_coro, {}))
		{
		}

		~LazyTask()
		{
			if (m_coro)
			{
				// 不允许异步任务未完成时销毁本类
				if (!m_coro.done())
				{
					std::unreachable();
				}
				m_coro.destroy();
			}
		}

		LazyTask& operator=(const LazyTask&) = delete;
		LazyTask& operator=(LazyTask&& that) = delete;

		template <typename E>
			requires requires(const E& func)
			{
				func(GuaranteedResolver<T>{});
			}
		static LazyTask create(E executor)
		{
			struct ExecAwaiter
			{
				const E& exec;
				GuaranteedResolver<T> resolver{new Resolver<T>};

				bool await_ready() noexcept
				{
					return false;
				}

				void await_suspend(std::coroutine_handle<promise_type> cont)
				{
					resolver->setNextCoroutineHandle(cont);
					auto resolveGuard = resolver;
					try
					{
						exec(resolveGuard);
					}
					catch (...)
					{
						resolveGuard->reject(std::current_exception());
					}
				}

				decltype(auto) await_resume()
				{
					return resolver->getValue();
				}
			};

			co_return co_await ExecAwaiter{executor};
		}

		static LazyTask reject(std::exception_ptr e)
		{
			std::rethrow_exception(e);
			// ReSharper disable CppUnreachableCode
			std::unreachable();
			// 为了让这个函数成为协程,必须有协程相关关键字
			co_return Resolver<T>{}.getValue();
			// ReSharper restore CppUnreachableCode
		}

		struct Awaiter
		{
			std::coroutine_handle<promise_type> coro;

			bool await_ready() noexcept
			{
				return !coro || coro.done();
			}

			template <typename U>
				requires requires
				{
					std::declval<U&&>().getCancellationToken();
				}
			std::coroutine_handle<> await_suspend(std::coroutine_handle<U> cont)
			{
				coro.promise().setCancellationToken(cont.promise().getCancellationToken());
				return awaitSuspendImpl(cont);
			}

			std::coroutine_handle<> await_suspend(std::coroutine_handle<> cont)
			{
				return awaitSuspendImpl(cont);
			}

			std::coroutine_handle<> awaitSuspendImpl(std::coroutine_handle<> cont)
			{
				auto& promise = coro.promise();
				if (promise.m_cont)
				{
					throw std::runtime_error("this task is already awaited by other");
				}
				promise.m_cont = cont;
				return coro;
			}

			decltype(auto) await_resume()
			{
				if (!coro)
				{
					throw std::runtime_error("no task running or it is already been moved");
				}
				auto& promise = coro.promise();
				return promise.getValue();
			}
		};

		Awaiter operator co_await() & noexcept
		{
			return Awaiter{m_coro};
		}

		struct AwaiterOwned
		{
			LazyTask task;

			bool await_ready() noexcept
			{
				return !task.m_coro || task.m_coro.done();
			}

			template <typename U>
				requires requires
				{
					std::declval<U&&>().getCancellationToken();
				}
			std::coroutine_handle<> await_suspend(std::coroutine_handle<U> cont)
			{
				task.m_coro.promise().setCancellationToken(cont.promise().getCancellationToken());
				return awaitSuspendImpl(cont);
			}

			std::coroutine_handle<> await_suspend(std::coroutine_handle<> cont)
			{
				return awaitSuspendImpl(cont);
			}

			std::coroutine_handle<> awaitSuspendImpl(std::coroutine_handle<> cont)
			{
				auto& promise = task.m_coro.promise();
				if (promise.m_cont)
				{
					throw std::runtime_error("this task is already awaited by other");
				}
				promise.m_cont = cont;
				return task.m_coro;
			}

			decltype(auto) await_resume()
			{
				if (!task.m_coro)
				{
					throw std::runtime_error("no task running or it is already been moved");
				}
				return static_cast<promise_type&&>(task.m_coro.promise()).getValue();
			}
		};

		AwaiterOwned operator co_await() && noexcept
		{
			return AwaiterOwned{std::move(*this)};
		}

		decltype(auto) syncAwait()
		{
			SyncLatch syncLatch;
			[](LazyTask& self, SyncLatch& latch) noexcept -> OnewayTask
			{
				ArriveOnExit guard(latch);
				co_await Awaiter{self.m_coro};
			}(*this, syncLatch);
			syncLatch.wait();

			return m_coro.promise().getValue();
		}

		constexpr explicit operator bool() const noexcept
		{
			return static_cast<bool>(m_coro);
		}

		[[nodiscard]] constexpr bool operator==(const LazyTask& other) noexcept
		{
			return m_coro == other.m_coro;
		}

	private:
		friend class LazyTaskPromise<T>;
		friend struct AwaiterOwned;

		explicit LazyTask(std::coroutine_handle<promise_type> coro) noexcept : m_coro(coro)
		{
		}

		std::coroutine_handle<promise_type> m_coro = nullptr;
	};
}
