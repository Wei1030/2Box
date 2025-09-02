export module Hook:Iphlpapi;

import "sys_defs.h";
import std;
import :Core;
import GlobalData;

namespace hook
{
	template <auto Trampoline>
	ULONG WINAPI GetAdaptersInfo(_Out_writes_bytes_opt_(*SizePointer) PIP_ADAPTER_INFO AdapterInfo, _Inout_ PULONG SizePointer)
	{
		const ULONG result = Trampoline(AdapterInfo, SizePointer);
		if (result == ERROR_SUCCESS)
		{
			PIP_ADAPTER_INFO pCurrAddresses = AdapterInfo;
			while (pCurrAddresses)
			{
				if (pCurrAddresses->AddressLength != 0)
				{
					BYTE address[MAX_ADAPTER_ADDRESS_LENGTH]{};
					const std::uint64_t flag = global::Data::get().envFlag();
					static_assert(sizeof(address) == sizeof(flag));
					static_assert(sizeof(pCurrAddresses->Address) == sizeof(flag));
					const UINT length = std::min(pCurrAddresses->AddressLength, static_cast<UINT>(sizeof(address)));
					memcpy(address, &flag, length);

					for (UINT i = 0; i < length; ++i)
					{
						pCurrAddresses->Address[i] += address[i];
					}
				}

				pCurrAddresses = pCurrAddresses->Next;
			}
		}
		return result;
	}

	template <auto Trampoline>
	ULONG WINAPI GetAdaptersAddresses(_In_ ULONG Family, _In_ ULONG Flags, _Reserved_ PVOID Reserved,
	                                  _Out_writes_bytes_opt_(*SizePointer) PIP_ADAPTER_ADDRESSES AdapterAddresses, _Inout_ PULONG SizePointer)
	{
		const ULONG result = Trampoline(Family, Flags, Reserved, AdapterAddresses, SizePointer);
		if (result == ERROR_SUCCESS)
		{
			PIP_ADAPTER_ADDRESSES pCurrAddresses = AdapterAddresses;
			while (pCurrAddresses)
			{
				if (pCurrAddresses->PhysicalAddressLength != 0)
				{
					BYTE physicalAddress[MAX_ADAPTER_ADDRESS_LENGTH]{};
					const std::uint64_t flag = global::Data::get().envFlag();
					static_assert(sizeof(physicalAddress) == sizeof(flag));
					static_assert(sizeof(pCurrAddresses->PhysicalAddress) == sizeof(flag));
					const ULONG length = std::min(pCurrAddresses->PhysicalAddressLength, static_cast<ULONG>(sizeof(physicalAddress)));
					memcpy(physicalAddress, &flag, length);

					for (UINT i = 0; i < length; ++i)
					{
						pCurrAddresses->PhysicalAddress[i] += physicalAddress[i];
					}
				}

				pCurrAddresses = pCurrAddresses->Next;
			}
		}
		return result;
	}

	void hook_iphlpapi()
	{
		constexpr auto IPHLPAPI_LIB_NAME = utils::make_literal_name<L"iphlpapi.dll">();

		create_hook_by_func_type<IPHLPAPI_LIB_NAME, utils::make_literal_name<"GetAdaptersInfo">(), decltype(::GetAdaptersInfo)>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&GetAdaptersInfo<trampolineConst.value>};
		});
		create_hook_by_func_type<IPHLPAPI_LIB_NAME, utils::make_literal_name<"GetAdaptersAddresses">(), decltype(::GetAdaptersAddresses)>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&GetAdaptersAddresses<trampolineConst.value>};
		});
	}
}
