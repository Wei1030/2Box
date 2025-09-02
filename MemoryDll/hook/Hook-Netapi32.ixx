export module Hook:Netapi32;

import "sys_defs.h";
import :Core;
import GlobalData;

namespace hook
{
	template <auto Trampoline>
	UCHAR APIENTRY Netbios(PNCB pNcb)
	{
		if (pNcb && (pNcb->ncb_command == NCBASTAT || pNcb->ncb_command == (NCBASTAT | ASYNCH)))
		{
			if (pNcb->ncb_buffer && pNcb->ncb_length >= sizeof(ADAPTER_STATUS))
			{
				const bool bIsAsync = pNcb->ncb_command & ASYNCH;
				auto post = pNcb->ncb_post;
				auto event = pNcb->ncb_event;
				if (bIsAsync)
				{
					pNcb->ncb_command &= ~ASYNCH;
					pNcb->ncb_post = nullptr;
					if (pNcb->ncb_event)
					{
						ResetEvent(pNcb->ncb_event);
						pNcb->ncb_event = nullptr;
					}
				}
				const UCHAR uRetCode = Trampoline(pNcb);
				if (bIsAsync)
				{
					pNcb->ncb_command |= ASYNCH;
					pNcb->ncb_post = post;
					pNcb->ncb_event = event;
				}
				if (uRetCode == NRC_GOODRET)
				{
					UCHAR adapterAddress[6]{};
					const std::uint64_t flag = global::Data::get().envFlag();
					static_assert(sizeof(adapterAddress) <= sizeof(flag));
					memcpy(adapterAddress, &flag, sizeof(adapterAddress));

					ADAPTER_STATUS* pAdapter = reinterpret_cast<ADAPTER_STATUS*>(pNcb->ncb_buffer);
					static_assert(sizeof(adapterAddress) == sizeof(pAdapter->adapter_address));
					for (UINT i = 0; i < 6; ++i)
					{
						pAdapter->adapter_address[i] += adapterAddress[i];
					}

					if (bIsAsync)
					{
						if (post)
						{
							post(pNcb);
						}
						else if (event)
						{
							SetEvent(event);
						}
						pNcb->ncb_retcode = NRC_PENDING;
						return NRC_PENDING;
					}
				}
				return uRetCode;
			}
		}
		return Trampoline(pNcb);
	}

	void hook_netapi32()
	{
		constexpr auto NETAPI32_LIB_NAME = utils::make_literal_name<L"netapi32.dll">();
		
		create_hook_by_func_type<NETAPI32_LIB_NAME, utils::make_literal_name<"Netbios">(), decltype(::Netbios)>().setHookFromGetter([&](auto trampolineConst)
		{
			return HookInfo{&Netbios<trampolineConst.value>};
		});
	}
}
