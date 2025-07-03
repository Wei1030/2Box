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
		std::coroutine_handle<> m_cont;

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
			except = ep;
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
				std::terminate();
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
				std::terminate();
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
				std::terminate();
			}
		}

		//////////////////////////////////////////
		/// 用于为回调式接口转为协程式的帮助类
		struct Resolver
		{
			std::coroutine_handle<PromiseTmplBase> coro = nullptr;
			mutable std::atomic<bool> settled = false;

			void resolve(T val) const
				requires std::is_reference_v<T>
			{
				if (settled.exchange(true, std::memory_order_relaxed))
				{
					return;
				}
				coro.promise().return_value(val);
				coro.resume();
			}

			template <typename U>
				requires (!std::is_reference_v<T> && std::convertible_to<U, T> && std::constructible_from<T, U>)
			void resolve(U&& val) const
			{
				if (settled.exchange(true, std::memory_order_relaxed))
				{
					return;
				}
				coro.promise().return_value(std::forward<U>(val));
				coro.resume();
			}

			void reject(std::exception_ptr ep) const
			{
				if (settled.exchange(true, std::memory_order_relaxed))
				{
					return;
				}
				coro.promise().set_exception(ep);
				coro.resume();
			}

			~Resolver()
			{
				if (settled.exchange(true, std::memory_order_relaxed))
				{
					return;
				}
				coro.promise().set_exception(std::make_exception_ptr(std::runtime_error("no one resolve")));
				coro.resume();
			}
		};

		std::shared_ptr<Resolver> makeResolver()
		{
			return std::make_shared<Resolver>(std::coroutine_handle<PromiseTmplBase>::from_promise(*this));
		}

	private:
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
			except = ep;
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

		//////////////////////////////////////////
		/// 用于为回调式接口转为协程式的帮助类
		struct Resolver
		{
			std::coroutine_handle<PromiseTmplBase> coro = nullptr;
			mutable std::atomic<bool> settled = false;

			void resolve() const
			{
				if (settled.exchange(true, std::memory_order_relaxed))
				{
					return;
				}
				coro.resume();
			}

			void reject(std::exception_ptr ep) const
			{
				if (settled.exchange(true, std::memory_order_relaxed))
				{
					return;
				}
				coro.promise().set_exception(ep);
				coro.resume();
			}

			~Resolver()
			{
				if (settled.exchange(true, std::memory_order_relaxed))
				{
					return;
				}
				coro.promise().set_exception(std::make_exception_ptr(std::runtime_error("no one reject")));
				coro.resume();
			}
		};

		std::shared_ptr<Resolver> makeResolver()
		{
			return std::make_shared<Resolver>(std::coroutine_handle<PromiseTmplBase>::from_promise(*this));
		}

	private:
		union
		{
			std::exception_ptr except;
		};
	};

	// promise指针代理类
	template <class T>
		requires requires(T* p)
		{
			p->addRef();
			p->release();
		}
	class PromisePtr
	{
	private:
		typedef PromisePtr ThisType;
		T* m_px;

	public:
		typedef T value_type;

		PromisePtr() noexcept : m_px(nullptr)
		{
		}

		PromisePtr(T* p) : m_px(p)
		{
			if (m_px != nullptr)
			{
				m_px->addRef();
			}
		}

		PromisePtr(PromisePtr const& rhs) : m_px(rhs.m_px)
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

		~PromisePtr()
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

		PromisePtr& operator=(T* rhs)
		{
			ThisType(rhs).swap(*this);
			return *this;
		}

		PromisePtr& operator=(PromisePtr const& rhs)
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
}
