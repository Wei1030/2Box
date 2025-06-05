module;
// 为了看到win7以后的win32 api定义，不引用framework.h而是单独包含windows.h
#include <SDKDDKVer.h>
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
export module DynamicWin32Api;

import std;
import Utility.LiteralName;

namespace win32_api
{
	template <typename CharType, CharType... C>
	using LiteralName = utils::LiteralName<CharType, C...>;
	
	template <LiteralName LibName>
	class LibLoader
	{
	public:
		static LibLoader& getInstance()
		{
			static LibLoader loader;
			return loader;
		}

		static HMODULE getModule()
		{
			return getInstance().m_lib;
		}

	private:
		LibLoader()
		{
			m_lib = LoadLibraryW(LibName.data);
		}

		~LibLoader()
		{
			if (m_lib)
			{
				FreeLibrary(m_lib);
			}
		}

	private:
		HMODULE m_lib{nullptr};
	};

	template <LiteralName LibName, LiteralName ApiName, typename ApiType>
		requires std::is_function_v<ApiType>
	struct ApiProxy
	{
		using ApiPtrType = std::add_pointer_t<ApiType>;
		ApiPtrType procAddr{nullptr};

		ApiPtrType get() const
		{
			struct InitHelper
			{
				InitHelper(ApiPtrType& apiAddr, std::string_view apiName)
				{
					apiAddr = reinterpret_cast<ApiPtrType>(GetProcAddress(LibLoader<LibName>::getModule(), apiName.data()));
				}
			};
			static InitHelper initHelper{const_cast<ApiProxy*>(this)->procAddr, ApiName.data};
			return procAddr;
		}

		explicit operator bool() const
		{
			return this->get() != nullptr;
		}

		template <typename... Args>
			requires std::is_invocable_r_v<
				std::invoke_result_t<ApiPtrType, Args...>,
				ApiPtrType,
				Args...
			>
		auto operator()(Args&&... args) const -> decltype(auto)
		{
			return std::invoke(this->get(), std::forward<Args>(args)...);
		}
	};

	inline constexpr auto KERNEL32_LIB_NAME = utils::make_literal_name<L"Kernel32">();
	inline constexpr auto USER32_LIB_NAME = utils::make_literal_name<L"User32">();
	
	// 微软文档化的,头文件中直接有函数定义的, 用这个宏
#define DECL_WIN32_API(libName, apiName) \
	inline ApiProxy<libName, utils::make_literal_name<#apiName>(), decltype(::apiName)> apiName
	
	// 微软未文档化的,头文件中找不到函数定义的, 用这个宏,但需要自己额外定义一下api形状传入
#define DECL_WIN32_API_BY_API_SHAPE(libName, apiName, apiShape) \
	inline ApiProxy<libName, utils::make_literal_name<#apiName>(), apiShape> apiName
	
	
	export
	{
		DECL_WIN32_API(USER32_LIB_NAME, GetDpiForWindow);
	}
}
