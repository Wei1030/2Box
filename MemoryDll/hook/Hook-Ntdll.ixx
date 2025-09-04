// ReSharper disable CppInconsistentNaming
module;
#include <ntstatus.h>
export module Hook:Ntdll;

import "sys_defs.h";
import :Core;
import std;
import GlobalData;
import Utility.SystemInfo;
import RpcClient;
import DynamicWin32Api;

namespace hook
{
	enum class ChangePolicy : std::uint8_t
	{
		ForceChange,
		TryToChange
	};

	template <auto trampoline, ChangePolicy Policy, typename... Args>
	NTSTATUS NTAPI ChangeObjNameThenTrampoline(POBJECT_ATTRIBUTES ObjectAttributes, Args&&... args)
	{
		if (ObjectAttributes && ObjectAttributes->ObjectName && ObjectAttributes->ObjectName->Buffer && ObjectAttributes->ObjectName->Length)
		{
			const PUNICODE_STRING pOldName = ObjectAttributes->ObjectName;
			std::wstring strNewName = std::format(L"{}{}", std::wstring_view{pOldName->Buffer, pOldName->Length / sizeof(wchar_t)}, global::Data::get().envFlagName());
			UNICODE_STRING newObjName;
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

	//////////////////////////////////////////////////////////////////////////
	//event
	template <auto trampoline>
	NTSTATUS NTAPI NtCreateEvent(OUT PHANDLE EventHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, IN int EventType, IN BOOLEAN InitialState)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::ForceChange>(ObjectAttributes, EventHandle, DesiredAccess, ObjectAttributes, EventType, InitialState);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtOpenEvent(OUT PHANDLE EventHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::TryToChange>(ObjectAttributes, EventHandle, DesiredAccess, ObjectAttributes);
	}

	//////////////////////////////////////////////////////////////////////////
	//mutant
	template <auto trampoline>
	NTSTATUS NTAPI NtCreateMutant(OUT PHANDLE MutantHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, IN BOOLEAN InitialOwner)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::ForceChange>(ObjectAttributes, MutantHandle, DesiredAccess, ObjectAttributes, InitialOwner);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtOpenMutant(OUT PHANDLE MutantHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::TryToChange>(ObjectAttributes, MutantHandle, DesiredAccess, ObjectAttributes);
	}

	//////////////////////////////////////////////////////////////////////////
	//section
	template <auto trampoline>
	NTSTATUS NTAPI NtCreateSection(OUT PHANDLE SectionHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes,
	                               IN PLARGE_INTEGER SectionSize OPTIONAL, IN ULONG Protect, IN ULONG Attributes, IN HANDLE FileHandle)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::ForceChange>(ObjectAttributes, SectionHandle, DesiredAccess, ObjectAttributes, SectionSize, Protect, Attributes, FileHandle);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtOpenSection(OUT PHANDLE SectionHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::TryToChange>(ObjectAttributes, SectionHandle, DesiredAccess, ObjectAttributes);
	}

	//////////////////////////////////////////////////////////////////////////
	//semaphore
	template <auto trampoline>
	NTSTATUS NTAPI NtCreateSemaphore(OUT PHANDLE SemaphoreHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, IN LONG InitialCount, IN LONG MaximumCount)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::ForceChange>(ObjectAttributes, SemaphoreHandle, DesiredAccess, ObjectAttributes, InitialCount, MaximumCount);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtOpenSemaphore(OUT PHANDLE SemaphoreHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::TryToChange>(ObjectAttributes, SemaphoreHandle, DesiredAccess, ObjectAttributes);
	}

	//////////////////////////////////////////////////////////////////////////
	//timer
	template <auto trampoline>
	NTSTATUS NTAPI NtCreateTimer(OUT PHANDLE TimerHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes, IN int TimerType)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::ForceChange>(ObjectAttributes, TimerHandle, DesiredAccess, ObjectAttributes, TimerType);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtOpenTimer(OUT PHANDLE TimerHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::TryToChange>(ObjectAttributes, TimerHandle, DesiredAccess, ObjectAttributes);
	}

	//////////////////////////////////////////////////////////////////////////
	//job
	template <auto trampoline>
	NTSTATUS NTAPI NtCreateJobObject(OUT PHANDLE JobHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::ForceChange>(ObjectAttributes, JobHandle, DesiredAccess, ObjectAttributes);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtOpenJobObject(OUT PHANDLE JobHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::TryToChange>(ObjectAttributes, JobHandle, DesiredAccess, ObjectAttributes);
	}

	//////////////////////////////////////////////////////////////////////////
	//named pipe
	template <auto trampoline>
	NTSTATUS NTAPI NtCreateNamedPipeFile(_Out_ PHANDLE FileHandle, _In_ ULONG DesiredAccess, _In_ POBJECT_ATTRIBUTES ObjectAttributes,
	                                     _Out_ PIO_STATUS_BLOCK IoStatusBlock, _In_ ULONG ShareAccess, _In_ ULONG CreateDisposition,
	                                     _In_ ULONG CreateOptions, _In_ ULONG NamedPipeType, _In_ ULONG ReadMode,
	                                     _In_ ULONG CompletionMode, _In_ ULONG MaximumInstances, _In_ ULONG InboundQuota,
	                                     _In_ ULONG OutboundQuota, _In_opt_ PLARGE_INTEGER DefaultTimeout)
	{
		return ChangeObjNameThenTrampoline<trampoline, ChangePolicy::ForceChange>(ObjectAttributes, FileHandle, DesiredAccess, ObjectAttributes,
		                                                                          IoStatusBlock, ShareAccess, CreateDisposition,
		                                                                          CreateOptions, NamedPipeType, ReadMode,
		                                                                          CompletionMode, MaximumInstances, InboundQuota,
		                                                                          OutboundQuota, DefaultTimeout);
	}

	template <auto trampoline, typename... Args>
	NTSTATUS NTAPI MyCreateOrOpenFile(POBJECT_ATTRIBUTES ObjectAttributes, Args&&... args)
	{
		NTSTATUS ret = STATUS_ACCESS_DENIED;
		do
		{
			if (!ObjectAttributes)
			{
				break;
			}
			if (ObjectAttributes->RootDirectory)
			{
				// 需要考虑吗?
				break;
			}
			if (!ObjectAttributes->ObjectName
				|| !ObjectAttributes->ObjectName->Buffer
				|| !ObjectAttributes->ObjectName->Length)
			{
				break;
			}

			std::wstring_view strCmpName(ObjectAttributes->ObjectName->Buffer, ObjectAttributes->ObjectName->Length / sizeof(wchar_t));
			//管道;
			if (strCmpName.starts_with(L"\\??\\pipe\\"))
			{
				/* \??\pipe\ */
				std::wstring strNewName = std::format(L"{}{}", strCmpName, global::Data::get().envFlagName());
				const PUNICODE_STRING pOldName = ObjectAttributes->ObjectName;
				UNICODE_STRING newObjName;
				newObjName.Buffer = strNewName.data();
				newObjName.Length = newObjName.MaximumLength = static_cast<USHORT>(strNewName.length() * sizeof(wchar_t));
				ObjectAttributes->ObjectName = &newObjName;
				ret = trampoline(std::forward<Args>(args)...);
				ObjectAttributes->ObjectName = pOldName;
				break;
			}
			// 符合条件的特定路径重定向
			if (std::optional<std::wstring> result = global::Data::get().redirectKnownFolderPath(strCmpName))
			{
				const PUNICODE_STRING pOldName = ObjectAttributes->ObjectName;
				UNICODE_STRING newObjName;
				newObjName.Buffer = result.value().data();
				newObjName.Length = newObjName.MaximumLength = static_cast<USHORT>(result.value().length() * sizeof(wchar_t));
				ObjectAttributes->ObjectName = &newObjName;
				ret = trampoline(std::forward<Args>(args)...);
				ObjectAttributes->ObjectName = pOldName;
				return ret;
			}
		}
		while (false);

		if (!NT_SUCCESS(ret))
		{
			ret = trampoline(std::forward<Args>(args)...);
		}
		return ret;
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtCreateFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, IN POBJECT_ATTRIBUTES ObjectAttributes,
	                            OUT PIO_STATUS_BLOCK IoStatusBlock, IN PLARGE_INTEGER AllocationSize OPTIONAL, IN ULONG FileAttributes,
	                            IN ULONG ShareAccess, IN ULONG CreateDisposition, IN ULONG CreateOptions,
	                            IN PVOID EaBuffer OPTIONAL, IN ULONG EaLength)
	{
		return MyCreateOrOpenFile<trampoline>(ObjectAttributes, FileHandle, DesiredAccess, ObjectAttributes,
		                                      IoStatusBlock, AllocationSize, FileAttributes, ShareAccess,
		                                      CreateDisposition, CreateOptions, EaBuffer, EaLength);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtOpenFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess,
	                          IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PIO_STATUS_BLOCK IoStatusBlock,
	                          IN ULONG ShareAccess, IN ULONG OpenOptions)
	{
		return MyCreateOrOpenFile<trampoline>(ObjectAttributes, FileHandle, DesiredAccess, ObjectAttributes,
		                                      IoStatusBlock, ShareAccess, OpenOptions);
	}

	std::unordered_set<std::uint64_t> GetAllProcessInEnv()
	{
		std::unordered_set<std::uint64_t> allProc;
		try
		{
			const rpc::ClientDefault c;
			std::uint64_t pids[rpc::MAX_PID_COUNT]{};
			std::uint32_t count = rpc::MAX_PID_COUNT;
			c.getAllProcessIdInEnv(global::Data::get().envFlag(), pids, &count);
			allProc.reserve(count);
			for (std::uint32_t i = 0; i < count; ++i)
			{
				allProc.insert(pids[i]);
			}
		}
		catch (...)
		{
		}
		allProc.insert(GetCurrentProcessId());
		return allProc;
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass, IN OUT PVOID SystemInformation, IN ULONG SystemInformationLength, OUT PULONG ReturnLength OPTIONAL)
	{
		const NTSTATUS ret = trampoline(SystemInformationClass, SystemInformation, SystemInformationLength, ReturnLength);
		if (!NT_SUCCESS(ret) || !SystemInformation)
		{
			return ret;
		}

		if (SystemInformationClass == SystemProcessInformation)
		{
			const std::unordered_set<std::uint64_t> allProc = GetAllProcessInEnv();
			PSYSTEM_PROCESS_INFORMATION pIndex = static_cast<PSYSTEM_PROCESS_INFORMATION>(SystemInformation);
			PSYSTEM_PROCESS_INFORMATION pShow = pIndex;

			do
			{
				if (pIndex->UniqueProcessId && !allProc.contains(reinterpret_cast<ULONG_PTR>(pIndex->UniqueProcessId)))
				{
					if (pIndex->NextEntryOffset)
					{
						pShow->NextEntryOffset += pIndex->NextEntryOffset;
					}
					else
					{
						pShow->NextEntryOffset = 0;
					}
				}
				else
				{
					pShow = pIndex;
				}

				if (pIndex->NextEntryOffset)
				{
					pIndex = reinterpret_cast<PSYSTEM_PROCESS_INFORMATION>(reinterpret_cast<char*>(pIndex) + pIndex->NextEntryOffset);
				}
				else
				{
					pIndex = nullptr;
				}
			}
			while (pIndex);
		}
		return ret;
	}

	//////////////////////////////////////////////////////////////////////////
	//Reg key
	typedef enum _KEY_INFORMATION_CLASS
	{
		KeyBasicInformation,
		KeyNodeInformation,
		KeyFullInformation,
		KeyNameInformation,
		KeyCachedInformation,
		KeyFlagsInformation,
		KeyVirtualizationInformation,
		KeyHandleTagsInformation,
		KeyTrustInformation,
		KeyLayerInformation,
		MaxKeyInfoClass
	} KEY_INFORMATION_CLASS;

	typedef struct _KEY_NAME_INFORMATION
	{
		ULONG NameLength;
		WCHAR Name[1];
	} KEY_NAME_INFORMATION, *PKEY_NAME_INFORMATION;

	typedef enum _KEY_VALUE_INFORMATION_CLASS
	{
		KeyValueBasicInformation,
		KeyValueFullInformation,
		KeyValuePartialInformation,
		KeyValueFullInformationAlign64,
		KeyValuePartialInformationAlign64,
		KeyValueLayerInformation,
		MaxKeyValueInfoClass
	} KEY_VALUE_INFORMATION_CLASS;

	inline win32_api::ApiProxy<utils::make_literal_name<L"ntdll">(), utils::make_literal_name<"NtQueryKey">(), NTSTATUS (NTAPI)(_In_ HANDLE KeyHandle,
	                                                                                                                            _In_ KEY_INFORMATION_CLASS KeyInformationClass,
	                                                                                                                            _Out_writes_bytes_opt_(Length) PVOID KeyInformation,
	                                                                                                                            _In_ ULONG Length,
	                                                                                                                            _Out_ PULONG ResultLength)> NtQueryKey;

	std::vector<std::byte> GetKeyNameInformation(HANDLE KeyHandle)
	{
		std::vector<std::byte> buffer;
		if (!KeyHandle)
		{
			return buffer;
		}
		ULONG size = 0;
		NtQueryKey(KeyHandle, KeyNameInformation, nullptr, 0, &size);
		if (size == 0)
		{
			return buffer;
		}
		buffer.resize(size);
		if (!NT_SUCCESS(NtQueryKey(KeyHandle, KeyNameInformation, buffer.data(), size, &size)))
		{
			buffer.clear();
			return buffer;
		}
		return buffer;
	}

	std::wstring GetKeyName(HANDLE KeyHandle)
	{
		std::wstring fullName;
		std::vector<std::byte> buffer = GetKeyNameInformation(KeyHandle);
		if (buffer.empty())
		{
			return fullName;
		}
		const KEY_NAME_INFORMATION* ni = reinterpret_cast<KEY_NAME_INFORMATION*>(buffer.data());
		fullName = std::wstring_view{ni->Name, ni->NameLength / sizeof(wchar_t)};
		return fullName;
	}

	std::expected<HKEY, LSTATUS> RegCreateKeyExW(_In_ HKEY hKey, _In_opt_ LPCWSTR lpSubKey)
	{
		HKEY hResKey{nullptr};
		LSTATUS status = ::RegCreateKeyExW(hKey, lpSubKey,
		                                   0, nullptr, 0,
		                                   KEY_ALL_ACCESS, nullptr, &hResKey, nullptr);
		if (status == ERROR_SUCCESS)
		{
			return std::expected<HKEY, LSTATUS>{hResKey};
		}
		return std::unexpected{status};
	}

	std::expected<HKEY, LSTATUS> RegOpenKeyExW(_In_ HKEY hKey, _In_opt_ LPCWSTR lpSubKey)
	{
		HKEY hResKey{nullptr};
		LSTATUS status = ::RegOpenKeyExW(hKey, lpSubKey, 0, KEY_ALL_ACCESS, &hResKey);
		if (status == ERROR_SUCCESS)
		{
			return std::expected<HKEY, LSTATUS>{hResKey};
		}
		return std::unexpected{status};
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtSetValueKey(_In_ HANDLE KeyHandle, _In_ PUNICODE_STRING ValueName, _In_opt_ ULONG TitleIndex,
	                             _In_ ULONG Type, _In_reads_bytes_opt_(DataSize) PVOID Data, _In_ ULONG DataSize)
	{
		// TODO:
		// 暂时先允许写入真实注册表
		// 测试发现一些写入如果不写入真实注册表程序会变的很卡，不知道为什么。也许一些系统操作必须写入，其对应的查询又貌似不走NtQueryValueKey
		// 暂时不研究过滤了，写入也不影响啥，又不是安全软件。
		const NTSTATUS retStatus = trampoline(KeyHandle, ValueName, TitleIndex, Type, Data, DataSize);
		if (!NT_SUCCESS(retStatus))
		{
			return retStatus;
		}
		// 额外往app env key写入，已供后续优先查询，这样每个环境中都有一份自己的虚拟注册表了
		const std::wstring keyName = GetKeyName(KeyHandle);
		if (keyName.size() && !global::is_app_key_name(keyName))
		{
			const std::expected<HKEY, LSTATUS> result =
				RegCreateKeyExW(global::Data::get().appKey(), global::remove_leading_backslashes_sv(keyName).data())
				.and_then([&](HKEY hKey)-> std::expected<HKEY, LSTATUS>
				{
					const NTSTATUS status = trampoline(hKey, ValueName, TitleIndex, Type, Data, DataSize);
					RegCloseKey(hKey);
					if (!NT_SUCCESS(status))
					{
						return std::unexpected{status};
					}
					return std::expected<HKEY, LSTATUS>{HKEY{}};
				});
			// if (result)
			// {
			// 	return STATUS_SUCCESS;
			// }
		}
		return retStatus;
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtQueryValueKey(_In_ HANDLE KeyHandle,
	                               _In_ PUNICODE_STRING ValueName,
	                               _In_ KEY_VALUE_INFORMATION_CLASS KeyValueInformationClass,
	                               _Out_writes_bytes_opt_(Length) PVOID KeyValueInformation,
	                               _In_ ULONG Length,
	                               _Out_ PULONG ResultLength)
	{
		// 优先从app env key查询，查询失败才从真实注册表查
		const std::wstring keyName = GetKeyName(KeyHandle);
		if (keyName.size() && !global::is_app_key_name(keyName))
		{
			const std::expected<HKEY, LSTATUS> result =
				RegOpenKeyExW(global::Data::get().appKey(), global::remove_leading_backslashes_sv(keyName).data())
				.and_then([&](HKEY hKey)-> std::expected<HKEY, LSTATUS>
				{
					const NTSTATUS status = trampoline(hKey, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
					RegCloseKey(hKey);
					if (!NT_SUCCESS(status))
					{
						return std::unexpected{status};
					}
					return std::expected<HKEY, LSTATUS>{HKEY{}};
				});
			if (result)
			{
				return STATUS_SUCCESS;
			}
			// 缓冲区不够不认为是失败，也直接返回。
			if (result.error() == STATUS_BUFFER_TOO_SMALL || result.error() == STATUS_BUFFER_OVERFLOW)
			{
				return result.error();
			}
		}
		return trampoline(KeyHandle, ValueName, KeyValueInformationClass, KeyValueInformation, Length, ResultLength);
	}

	template <auto trampoline>
	NTSTATUS NTAPI NtQueryMultipleValueKey(_In_ HANDLE KeyHandle,
	                                       _Inout_updates_(EntryCount) PKEY_VALUE_ENTRY ValueEntries,
	                                       _In_ ULONG EntryCount,
	                                       _Out_writes_bytes_(*BufferLength) PVOID ValueBuffer,
	                                       _Inout_ PULONG BufferLength,
	                                       _Out_opt_ PULONG RequiredBufferLength)
	{
		// 优先从app env key查询，查询失败才从真实注册表查
		const std::wstring keyName = GetKeyName(KeyHandle);
		if (keyName.size() && !global::is_app_key_name(keyName))
		{
			const std::expected<HKEY, LSTATUS> result =
				RegOpenKeyExW(global::Data::get().appKey(), global::remove_leading_backslashes_sv(keyName).data())
				.and_then([&](HKEY hKey)-> std::expected<HKEY, LSTATUS>
				{
					const NTSTATUS status = trampoline(hKey, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
					RegCloseKey(hKey);
					if (!NT_SUCCESS(status))
					{
						return std::unexpected{status};
					}
					return std::expected<HKEY, LSTATUS>{HKEY{}};
				});
			if (result)
			{
				return STATUS_SUCCESS;
			}
			// 缓冲区不够不认为是失败，也直接返回。
			if (result.error() == STATUS_BUFFER_TOO_SMALL || result.error() == STATUS_BUFFER_OVERFLOW)
			{
				return result.error();
			}
		}
		return trampoline(KeyHandle, ValueEntries, EntryCount, ValueBuffer, BufferLength, RequiredBufferLength);
	}

	void hook_ntdll()
	{
		constexpr auto NTDLL_LIB_NAME = utils::make_literal_name<L"ntdll.dll">();
		sys_info::SysDllMapHelper ntdllMapped = sys_info::get_ntdll_mapped();
		void* ntdllMappedAddress = ntdllMapped.memAddress();

#define CREATE_HOOK_BY_NAME(name) \
		create_hook_by_func_type<NTDLL_LIB_NAME, utils::make_literal_name<#name>(), decltype(name<nullptr>)>().setHookFromGetter([&](auto trampolineConst) \
		{ \
			return HookInfo{&name<trampolineConst.value>, ntdllMappedAddress}; \
		})

		CREATE_HOOK_BY_NAME(NtCreateEvent);
		CREATE_HOOK_BY_NAME(NtOpenEvent);
		CREATE_HOOK_BY_NAME(NtCreateMutant);
		CREATE_HOOK_BY_NAME(NtOpenMutant);
		CREATE_HOOK_BY_NAME(NtCreateSection);
		CREATE_HOOK_BY_NAME(NtOpenSection);
		CREATE_HOOK_BY_NAME(NtCreateSemaphore);
		CREATE_HOOK_BY_NAME(NtOpenSemaphore);
		CREATE_HOOK_BY_NAME(NtCreateTimer);
		CREATE_HOOK_BY_NAME(NtOpenTimer);
		CREATE_HOOK_BY_NAME(NtCreateJobObject);
		CREATE_HOOK_BY_NAME(NtOpenJobObject);
		CREATE_HOOK_BY_NAME(NtCreateNamedPipeFile);
		CREATE_HOOK_BY_NAME(NtCreateFile);
		CREATE_HOOK_BY_NAME(NtOpenFile);
		CREATE_HOOK_BY_NAME(NtQuerySystemInformation);

		// 简单做一个虚拟注册表：将所有的写入操作都copy一份到自己的环境中，查询优先查询虚拟环境中的，找不到的话再从真实注册表查
		CREATE_HOOK_BY_NAME(NtSetValueKey);
		CREATE_HOOK_BY_NAME(NtQueryValueKey);
		CREATE_HOOK_BY_NAME(NtQueryMultipleValueKey);
	}
}
