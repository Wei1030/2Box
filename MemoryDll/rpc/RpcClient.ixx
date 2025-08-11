// ReSharper disable CppFunctionalStyleCast
module;
#include "service_h.h"
export module RpcClient;

import std;

namespace rpc
{
	export constexpr unsigned int MAX_PID_COUNT = MAX_PIDS;
	
	class ClientBindingString
	{
	public:
		ClientBindingString() = default;

		~ClientBindingString()
		{
			if (m_pszStringBinding)
			{
				RpcStringFreeA(&m_pszStringBinding);
			}
		}

		ClientBindingString(const ClientBindingString&) = delete;
		ClientBindingString& operator=(const ClientBindingString&) = delete;

		ClientBindingString(ClientBindingString&& that) noexcept : m_pszStringBinding(std::exchange(that.m_pszStringBinding, nullptr))
		{
		}

		ClientBindingString& operator=(ClientBindingString&& that) noexcept
		{
			std::swap(m_pszStringBinding, that.m_pszStringBinding);
			return *this;
		}

		operator unsigned char*() const
		{
			return m_pszStringBinding;
		}

	protected:
		unsigned char* m_pszStringBinding{nullptr};
	};

	export class ClientAlpcBindingString : public ClientBindingString
	{
	public:
		explicit ClientAlpcBindingString(std::string_view endPoint)
		{
			RPC_STATUS status = RpcStringBindingComposeA(nullptr, RPC_CSTR("ncalrpc"), nullptr, RPC_CSTR(endPoint.data()), nullptr, &m_pszStringBinding);
			if (status != RPC_S_OK)
			{
				throw std::runtime_error(std::format("RpcStringBindingComposeA failed, return status: {}", status));
			}
		}
	};

	export class ClientBindingHandle
	{
	public:
		ClientBindingHandle() = default;

		explicit ClientBindingHandle(const ClientBindingString& bindingString)
		{
			RPC_STATUS status = RpcBindingFromStringBindingA(bindingString, &m_hBindingHandle);
			if (status != RPC_S_OK)
			{
				throw std::runtime_error(std::format("RpcBindingFromStringBindingA failed, return status: {}", status));
			}
		}

		ClientBindingHandle(const ClientBindingHandle& rhs)
		{
			if (rhs.m_hBindingHandle)
			{
				RPC_STATUS status = RpcBindingCopy(rhs.m_hBindingHandle, &m_hBindingHandle);
				if (status != RPC_S_OK)
				{
					throw std::runtime_error(std::format("RpcBindingCopy failed, return status: {}", status));
				}
			}
		}

		ClientBindingHandle(ClientBindingHandle&& that) noexcept : m_hBindingHandle(std::exchange(that.m_hBindingHandle, nullptr))
		{
		}

		~ClientBindingHandle()
		{
			if (m_hBindingHandle)
			{
				RpcBindingFree(&m_hBindingHandle);
			}
		}

		void swap(ClientBindingHandle& rhs) noexcept
		{
			const handle_t tmp = m_hBindingHandle;
			m_hBindingHandle = rhs.m_hBindingHandle;
			rhs.m_hBindingHandle = tmp;
		}

		friend void swap(ClientBindingHandle& c1, ClientBindingHandle& c2) noexcept
		{
			c1.swap(c2);
		}

		ClientBindingHandle& operator=(const ClientBindingHandle& rhs)
		{
			ClientBindingHandle(rhs).swap(*this);
			return *this;
		}

		ClientBindingHandle& operator=(ClientBindingHandle&& that) noexcept
		{
			std::swap(m_hBindingHandle, that.m_hBindingHandle);
			return *this;
		}

		handle_t getHandle() const
		{
			return m_hBindingHandle;
		}

		constexpr explicit operator bool() const noexcept
		{
			return m_hBindingHandle != nullptr;
		}

	private:
		handle_t m_hBindingHandle{nullptr};
	};

	export const ClientBindingHandle& get_default_binding_info()
	{
		static ClientBindingHandle defaultBindingInfo(ClientAlpcBindingString{"{63B40BDA-A2D1-4516-BDBB-E1E2A960D31E}2BoxServer"});
		return defaultBindingInfo;
	}

	template <typename Func, typename... Args>
	decltype(auto) rpc_call_wrapper(Func&& func, Args&&... args)
	{
		__try
		{
			return std::invoke(std::forward<Func>(func), std::forward<Args>(args)...);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			TerminateProcess(GetCurrentProcess(), RpcExceptionCode());
		}
	}

	export
	template <typename DerivedT>
	class ClientBase
	{
	public:
		handle_t handle() const
		{
			return static_cast<const DerivedT*>(this)->getHandle();
		}

		decltype(auto) injectToProcess(unsigned int pid, unsigned long long envFlag) const
		{
			return rpc_call_wrapper(&inject_to_process, handle(), pid, envFlag);
		}

		decltype(auto) getAllProcessIdInEnv(unsigned long long envFlag, unsigned long long pids[], unsigned int *count) const
		{
			return rpc_call_wrapper(&get_all_process_id_in_env, handle(), envFlag, pids, count);
		}
	};

	// 多线程使用同一个ClientDefault进行远程调用是线程安全的. see: https://learn.microsoft.com/en-us/windows/win32/rpc/rpc-binding-handle
	// 但是修改绑定信息需要自己做好同步, 所以一旦涉及修改操作，则所有操作都要考虑线程安全，包括远程调用和句柄的复制操作。
	// 所以如果需要修改绑定信息，建议不要使用ClientDefault，使用Client从一开始就从默认句柄复制出来
	export class ClientDefault : public ClientBase<ClientDefault>
	{
	public:
		ClientDefault() : m_handle(get_default_binding_info().getHandle())
		{
		}

		handle_t getHandle() const
		{
			return m_handle;
		}

	private:
		handle_t m_handle;
	};

	export class Client : public ClientBase<Client>
	{
	public:
		// Client自身复制时也会复制句柄。多个Client互不影响
		explicit Client(ClientBindingHandle handle) noexcept : m_handle(std::move(handle))
		{
		}

		handle_t getHandle() const
		{
			return m_handle.getHandle();
		}

	private:
		ClientBindingHandle m_handle;
	};
}
