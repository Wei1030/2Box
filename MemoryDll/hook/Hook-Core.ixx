module;
#include "sys_defs.h"
#include "detours.h"
export module Hook:Core;

import std;
import Utility.LiteralName;
import DynamicWin32Api;
import PELoader;

namespace hook
{
	class HookManager
	{
	public:
		static HookManager& instance()
		{
			static HookManager instance;
			return instance;
		}

		template <typename FuncPtrType>
		void addHook(std::add_pointer_t<FuncPtrType> source, FuncPtrType target)
		{
			m_hooks.emplace_back(HookInfo{reinterpret_cast<void**>(source), target});
		}

		void installAll()
		{
			DetourTransactionBegin();
			DetourUpdateThread(GetCurrentThread());

			for (const HookInfo& hook : m_hooks)
			{
				DetourAttach(hook.sourceAddr, hook.targetAddr);
			}

			if (const LONG error = DetourTransactionCommit(); error != NO_ERROR)
			{
				throw std::runtime_error(std::format("DetourTransactionCommit() failed with error {}", error));
			}

			std::vector<HookInfo> empty{};
			m_hooks.swap(empty);
		}

	private:
		HookManager()
		{
			m_hooks.reserve(100);
		}

		struct HookInfo
		{
			void** sourceAddr;
			void* targetAddr;
		};

		std::vector<HookInfo> m_hooks;
	};

	template <auto FuncPtr>
	struct FuncPtrHookIdentify
	{
		using FuncPtrType = decltype(FuncPtr);
		static constexpr const char* apiName = "unknown api name (by func ptr)";

		static constexpr FuncPtrType getFuncPtr()
		{
			return FuncPtr;
		}

		static constexpr FuncPtrType getFuncPtrByFile(void*)
		{
			return FuncPtr;
		}
	};

	template <typename CharType, CharType... C>
	using LiteralName = utils::LiteralName<CharType, C...>;

	template <LiteralName LibName, LiteralName ApiName, typename ApiType>
	struct NameHookIdentify
	{
		using FuncPtrType = std::add_pointer_t<ApiType>;
		static constexpr const char* apiName = ApiName.data;

		static FuncPtrType getFuncPtr()
		{
			return reinterpret_cast<FuncPtrType>(::GetProcAddress(win32_api::LibLoader<LibName>::getModule(), ApiName.data));
		}

		static FuncPtrType getFuncPtrByFile(void* fileMappedAddress)
		{
			void* hModule = reinterpret_cast<void*>(win32_api::LibLoader<LibName>::getModule());
			if (!hModule)
			{
				return nullptr;
			}
			const pe::Parser<pe::parser_flag::HasSectionAligned> parser{static_cast<char*>(fileMappedAddress)};
			const DWORD procRva = parser.getProcRVA(ApiName.data);
			if (!procRva)
			{
				return nullptr;
			}
			return reinterpret_cast<FuncPtrType>(static_cast<char*>(hModule) + procRva);
		}
	};

	template <typename FuncPtrType>
	struct HookInfo
	{
		explicit HookInfo(FuncPtrType inTargetFunc, void* inSourceFileMappedAddress = nullptr)
			: targetFunc(inTargetFunc), sourceFileMappedAddress(inSourceFileMappedAddress)
		{
		}

		FuncPtrType targetFunc;
		void* sourceFileMappedAddress;
	};

	template <typename IdentifyType>
	class Hook
	{
	public:
		using FuncPtrType = typename IdentifyType::FuncPtrType;

		struct Trampoline
		{
			static FuncPtrType funcAddress;

			template <typename... Args>
				requires std::is_invocable_r_v<
					std::invoke_result_t<FuncPtrType, Args...>,
					FuncPtrType,
					Args...
				>
			decltype(auto) operator()(Args&&... args) const
			{
				return std::invoke(funcAddress, std::forward<Args>(args)...);
			}
		};

		static constexpr Trampoline trampoline = Trampoline{};

		Trampoline setHook(FuncPtrType targetFunc)
		{
			Trampoline::funcAddress = IdentifyType::getFuncPtr();
			if (!Trampoline::funcAddress)
			{
				throw std::runtime_error{std::format("Failed to find function address for '{}'", IdentifyType::apiName)};
			}
			HookManager::instance().addHook(std::addressof(Trampoline::funcAddress), targetFunc);
			return trampoline;
		}

		Trampoline setHookFromMappedFile(void* fileMappedAddress, FuncPtrType targetFunc)
		{
			Trampoline::funcAddress = IdentifyType::getFuncPtrByFile(fileMappedAddress);
			if (!Trampoline::funcAddress)
			{
				throw std::runtime_error{std::format("Failed to find function address for '{}'", IdentifyType::apiName)};
			}
			HookManager::instance().addHook(std::addressof(Trampoline::funcAddress), targetFunc);
			return trampoline;
		}

		template <typename Getter>
			requires requires(Getter getter)
			{
				{ getter(std::integral_constant<Trampoline, trampoline>{}) } -> std::same_as<HookInfo<FuncPtrType>>;
			}
		Trampoline setHookFromGetter(Getter infoGetter)
		{
			const HookInfo<FuncPtrType> info = infoGetter(std::integral_constant<Trampoline, trampoline>{});
			if (info.sourceFileMappedAddress)
			{
				Trampoline::funcAddress = IdentifyType::getFuncPtrByFile(info.sourceFileMappedAddress);
			}
			else
			{
				Trampoline::funcAddress = IdentifyType::getFuncPtr();
			}
			if (!Trampoline::funcAddress)
			{
				throw std::runtime_error{std::format("Failed to find function address for '{}'", IdentifyType::apiName)};
			}
			HookManager::instance().addHook(std::addressof(Trampoline::funcAddress), info.targetFunc);
			return trampoline;
		}
	};

	template <typename IdentifyType>
	typename Hook<IdentifyType>::FuncPtrType Hook<IdentifyType>::Trampoline::funcAddress{nullptr};

	template <auto FuncPtr>
	auto create_hook_by_func_ptr()
	{
		return Hook<FuncPtrHookIdentify<FuncPtr>>{};
	}

	template <LiteralName LibName, LiteralName ApiName, typename ApiType>
	auto create_hook_by_func_type()
	{
		return Hook<NameHookIdentify<LibName, ApiName, ApiType>>{};
	}
}
