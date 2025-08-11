// ReSharper disable CppInconsistentNaming
export module Hook:Ntdll;

import "sys_defs.h";
import :Core;
import GlobalData;
import Utility.SystemInfo;


namespace hook
{
	enum class ChangePolicy : std::uint8_t
	{
		ForceChange,
		TryToChange
	};

	template <auto trampoline, ChangePolicy Policy, typename... Args>
	NTSTATUS NTAPI ChangeObjNameAndTrampoline(POBJECT_ATTRIBUTES ObjectAttributes, Args&&... args)
	{
		if (ObjectAttributes && ObjectAttributes->ObjectName && ObjectAttributes->ObjectName->Buffer && ObjectAttributes->ObjectName->Length)
		{
			const PUNICODE_STRING pOldName = ObjectAttributes->ObjectName;
			UNICODE_STRING newObjName;
			std::wstring strNewName = std::format(L"{}{}", std::wstring_view{pOldName->Buffer, pOldName->Length / sizeof(wchar_t)}, global::Data::get().envFlagName());
			newObjName.Buffer = strNewName.data();
			newObjName.Length = newObjName.MaximumLength = static_cast<USHORT>(strNewName.length() * sizeof(wchar_t));
			ObjectAttributes->ObjectName = &newObjName;
			const NTSTATUS ret = trampoline(std::forward<Args>(args)...);
			// restore to old name
			ObjectAttributes->ObjectName = pOldName;
			// 如果需要强制改名，则直接返回刚才的结果即可
			if constexpr (Policy == ChangePolicy::ForceChange)
			{
				return ret;
			}
			// 如果只是尝试改名
			else
			{
				// 那么如果改名后接口调用失败，则使用原名再试一次
				if (!NT_SUCCESS(ret))
				{
					return trampoline(std::forward<Args>(args)...);
				}
				return ret;
			}
		}
		// 匿名对象直接调用原函数即可
		return trampoline(std::forward<Args>(args)...);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtCreateEvent(OUT PHANDLE EventHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, IN int EventType, IN BOOLEAN InitialState)
	{
		return ChangeObjNameAndTrampoline<trampoline, ChangePolicy::ForceChange>(ObjectAttributes, EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtOpenEvent(OUT PHANDLE EventHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes)
	{
		return ChangeObjNameAndTrampoline<trampoline, ChangePolicy::TryToChange>(ObjectAttributes, EventHandle, DesiredAccess, ObjectAttributes);
	}

	void hook_ntdll()
	{
		constexpr auto NTDLL_LIB_NAME = utils::make_literal_name<L"ntdll.dll">();
		sys_info::SysDllMapHelper ntdllMapped = sys_info::get_ntdll_mapped();
		void* ntdllMappedAddress;

#define CREATE_HOOK_BY_NAME(name) \
		create_hook_by_func_type<NTDLL_LIB_NAME, utils::make_literal_name<#name>(), decltype(name<nullptr>)>().setHookFromGetter([&](auto trampolineConst) \
		{ \
			return HookInfo{&name<trampolineConst.value>, ntdllMappedAddress}; \
		})
		
		CREATE_HOOK_BY_NAME(NtCreateEvent);
		CREATE_HOOK_BY_NAME(NtOpenEvent);
	}
}
