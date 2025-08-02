export module Coroutine:PromiseBase;

import std;

namespace coro
{
	class PromiseBase
	{
	public:
		PromiseBase() = default;
		PromiseBase(const PromiseBase&) = delete;
		PromiseBase(PromiseBase&&) = delete;
		PromiseBase& operator=(const PromiseBase&) = delete;
		PromiseBase& operator=(PromiseBase&&) = delete;

	protected:
		enum class Discriminator : unsigned char
		{
			Empty,
			Exception,
			Data
		};

		Discriminator m_disc = Discriminator::Empty;
	};

	template <typename T>
	class PromiseTmplBase : public PromiseBase
	{
	public:
		// 特意不要初始化union成员
		// ReSharper disable CppPossiblyUninitializedMember
		PromiseTmplBase() noexcept
		// ReSharper restore CppPossiblyUninitializedMember
		{
		}

		~PromiseTmplBase()
		{
			switch (m_disc)
			{
			case Discriminator::Empty:
				break;
			case Discriminator::Data:
				std::destroy_at(std::addressof(data));
				break;
			case Discriminator::Exception:
				std::destroy_at(std::addressof(except));
				break;
			}
		}

		void return_value(T val) noexcept
			requires std::is_reference_v<T>
		{
			data = std::addressof(val);
			m_disc = Discriminator::Data;
		}

		template <typename U>
			requires (!std::is_reference_v<T> && std::convertible_to<U, T> && std::constructible_from<T, U>)
		void return_value(U&& val) noexcept(std::is_nothrow_constructible_v<U, T>)
		{
			std::construct_at(std::addressof(data), std::forward<U>(val));
			m_disc = Discriminator::Data;
		}

		void unhandled_exception() noexcept
		{
			std::construct_at(std::addressof(except), std::current_exception());
			m_disc = Discriminator::Exception;
		}

		void set_exception(std::exception_ptr ep) noexcept
		{
			std::construct_at(std::addressof(except), ep);
			m_disc = Discriminator::Exception;
		}

		decltype(auto) getValue() &
		{
			switch (m_disc)
			{
			case Discriminator::Data:
				if constexpr (std::is_reference_v<T>)
				{
					// T is a reference, data is a pointer
					return static_cast<T>(*data);
				}
				else
				{
					return static_cast<T&>(data);
				}
			case Discriminator::Exception:
				std::rethrow_exception(except);
			default:
				// "This can't happen?"
				std::unreachable();
			}
		}

		decltype(auto) getValue() &&
		{
			switch (m_disc)
			{
			case Discriminator::Data:
				if constexpr (std::is_reference_v<T>)
				{
					// T is a reference, data is a pointer
					return static_cast<T>(*data);
				}
				else
				{
					return T{std::move(data)};
				}
			case Discriminator::Exception:
				std::rethrow_exception(except);
			default:
				// "This can't happen?"
				std::unreachable();
			}
		}

		decltype(auto) copyValue()
		{
			switch (m_disc)
			{
			case Discriminator::Data:
				if constexpr (std::is_reference_v<T>)
				{
					// T is a reference, data is a pointer
					return static_cast<T>(*data);
				}
				else
				{
					return data;
				}
			case Discriminator::Exception:
				std::rethrow_exception(except);
			default:
				// "This can't happen?"
				std::unreachable();
			}
		}

	protected:
		union
		{
			std::exception_ptr except;
			std::conditional_t<std::is_reference_v<T>, std::add_pointer_t<T>, T> data;
		};
	};

	template <typename T>
		requires std::is_void_v<T>
	class PromiseTmplBase<T> : public PromiseBase
	{
	public:
		// 特意不要初始化union成员
		// ReSharper disable CppPossiblyUninitializedMember
		PromiseTmplBase() noexcept
		// ReSharper restore CppPossiblyUninitializedMember
		{
		}

		~PromiseTmplBase()
		{
			if (m_disc == Discriminator::Exception)
			{
				std::destroy_at(std::addressof(except));
			}
		}

		void return_void() noexcept
		{
		}

		void unhandled_exception() noexcept
		{
			std::construct_at(std::addressof(except), std::current_exception());
			m_disc = Discriminator::Exception;
		}

		void set_exception(std::exception_ptr ep) noexcept
		{
			std::construct_at(std::addressof(except), ep);
			m_disc = Discriminator::Exception;
		}

		void getValue() const
		{
			if (m_disc == Discriminator::Exception)
			{
				std::rethrow_exception(except);
			}
		}

		void copyValue() const
		{
			if (m_disc == Discriminator::Exception)
			{
				std::rethrow_exception(except);
			}
		}

	protected:
		union
		{
			std::exception_ptr except;
		};
	};

	//////////////////////////////////////////
	/// 用于为回调式接口转为协程式的帮助类
	template <typename T, typename DerivedT>
	class ResolverBase : public PromiseTmplBase<T>
	{
	public:
		void setNextCoroutineHandle(std::coroutine_handle<> cont) noexcept
		{
			m_cont = cont;
		}

		void reject(std::exception_ptr ep)
		{
			if (m_settled.exchange(true, std::memory_order_relaxed))
			{
				return;
			}
			this->set_exception(ep);
			m_cont.resume();
		}

		void rejectWithRuntimeError(std::string_view errorMsg)
		{
			if (m_settled.exchange(true, std::memory_order_relaxed))
			{
				return;
			}
			this->set_exception(std::make_exception_ptr(std::runtime_error{errorMsg.data()}));
			m_cont.resume();
		}

		void cancel()
		{
			if (m_settled.exchange(true, std::memory_order_relaxed))
			{
				return;
			}
			this->set_exception(std::make_exception_ptr(std::runtime_error{"operation canceled"}));
			m_cont.resume();
		}

		void abandon()
		{
			if (m_settled.exchange(true, std::memory_order_relaxed))
			{
				return;
			}
			this->set_exception(std::make_exception_ptr(std::runtime_error("no one resolve")));
			m_cont.resume();
		}

		void addRef() noexcept
		{
			m_refCount.fetch_add(1, std::memory_order_relaxed);
		}

		void release() noexcept
		{
			const std::uint32_t lastCount = m_refCount.fetch_sub(1, std::memory_order_acq_rel);
			if (lastCount == 2)
			{
				abandon();
			}
			else if (lastCount == 1)
			{
				delete static_cast<DerivedT*>(this);
			}
		}

	protected:
		mutable std::atomic<bool> m_settled = false;
		std::atomic<std::uint32_t> m_refCount{0};
		std::coroutine_handle<> m_cont = nullptr;
	};

	template <typename T>
	class Resolver : public ResolverBase<T, Resolver<T>>
	{
	public:
		void resolve(T val)
			requires (std::is_reference_v<T>)
		{
			if (this->m_settled.exchange(true, std::memory_order_relaxed))
			{
				return;
			}
			this->return_value(val);
			this->m_cont.resume();
		}

		template <typename U>
			requires (!std::is_reference_v<T> && std::convertible_to<U, T> && std::constructible_from<T, U>)
		void resolve(U&& val)
		{
			if (this->m_settled.exchange(true, std::memory_order_relaxed))
			{
				return;
			}
			this->return_value(std::forward<U>(val));
			this->m_cont.resume();
		}
	};

	template <>
	class Resolver<void> : public ResolverBase<void, Resolver<void>>
	{
	public:
		void resolve() const
		{
			if (m_settled.exchange(true, std::memory_order_relaxed))
			{
				return;
			}
			m_cont.resume();
		}
	};

	// promise指针代理类
	export
	template <class T>
	class PromisePtr
	{
		static_assert(noexcept(std::declval<T>().addRef()));
		static_assert(noexcept(std::declval<T>().release()));

	private:
		typedef PromisePtr ThisType;
		T* m_px;

	public:
		typedef T value_type;

		PromisePtr() noexcept : m_px(nullptr)
		{
		}

		PromisePtr(T* p) noexcept : m_px(p)
		{
			if (m_px != nullptr)
			{
				m_px->addRef();
			}
		}

		PromisePtr(PromisePtr const& rhs) noexcept : m_px(rhs.m_px)
		{
			if (m_px != nullptr)
			{
				m_px->addRef();
			}
		}

		PromisePtr(PromisePtr&& rhs) noexcept : m_px(rhs.m_px)
		{
			rhs.m_px = nullptr;
		}

		~PromisePtr() noexcept
		{
			if (m_px != nullptr)
			{
				m_px->release();
			}
		}

		void swap(PromisePtr& rhs) noexcept
		{
			T* tmp = m_px;
			m_px = rhs.m_px;
			rhs.m_px = tmp;
		}

		friend void swap(PromisePtr& p1, PromisePtr& p2) noexcept
		{
			p1.swap(p2);
		}

		PromisePtr& operator=(T* rhs) noexcept
		{
			ThisType(rhs).swap(*this);
			return *this;
		}

		PromisePtr& operator=(PromisePtr const& rhs) noexcept
		{
			ThisType(rhs).swap(*this);
			return *this;
		}

		PromisePtr& operator=(PromisePtr&& rhs) noexcept
		{
			ThisType(static_cast<PromisePtr&&>(rhs)).swap(*this);
			return *this;
		}

		T* get() const noexcept
		{
			return m_px;
		}

		T& operator*() const noexcept
		{
			return *m_px;
		}

		T* operator->() const noexcept
		{
			return m_px;
		}

		explicit operator bool() const noexcept
		{
			return m_px != nullptr;
		}

		bool operator!() const noexcept
		{
			return m_px == nullptr;
		}
	};

	export
	template <typename T>
	using GuaranteedResolver = PromisePtr<Resolver<T>>;

	export
	struct OnewayTask
	{
		struct promise_type
		{
			// ReSharper disable CppMemberFunctionMayBeStatic
			std::suspend_never initial_suspend() noexcept { return {}; }
			std::suspend_never final_suspend() noexcept { return {}; }

			void unhandled_exception() noexcept
			{
			}

			OnewayTask get_return_object() noexcept { return {}; }

			void return_void() noexcept
			{
			}

			// ReSharper restore CppMemberFunctionMayBeStatic
		};
	};

	export struct SyncLatch
	{
	public:
		void arrive() noexcept
		{
			m_flag.store(0, std::memory_order::release);
			m_flag.notify_one();
		}

		void wait() noexcept
		{
			while (m_flag.exchange(1, std::memory_order::acquire))
			{
				// wait until flag is not 1
				m_flag.wait(1, std::memory_order::relaxed);
			}
		}

	private:
		std::atomic<int> m_flag{1};
	};

	export struct ArriveOnExit
	{
		SyncLatch& latch;

		explicit ArriveOnExit(SyncLatch& l) noexcept : latch(l)
		{
		}

		ArriveOnExit(const ArriveOnExit&) = delete;
		ArriveOnExit& operator=(const ArriveOnExit&) = delete;
		ArriveOnExit(ArriveOnExit&& rhs) = delete;
		ArriveOnExit& operator=(ArriveOnExit&& rhs) = delete;

		~ArriveOnExit() noexcept
		{
			latch.arrive();
		}
	};
}
