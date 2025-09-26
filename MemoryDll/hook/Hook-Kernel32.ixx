export module Hook:Kernel32;

import "sys_defs.h";
import :Core;
import std;
import GlobalData;
import RpcClient;
import PELoader;
import Utility.SystemInfo;

namespace hook
{
	template <auto Trampoline>
	BOOL WINAPI WaitNamedPipeA(__in LPCSTR lpNamedPipeName,__in DWORD nTimeOut)
	{
		if (lpNamedPipeName)
		{
			if (const BOOL bRet = Trampoline(std::format("{}{}", lpNamedPipeName, global::Data::get().envFlagNameA()).c_str(), nTimeOut))
			{
				return bRet;
			}
		}
		return Trampoline(lpNamedPipeName, nTimeOut);
	}

	template <auto Trampoline>
	BOOL WINAPI WaitNamedPipeW(__in LPCWSTR lpNamedPipeName, __in DWORD nTimeOut)
	{
		if (lpNamedPipeName)
		{
			if (const BOOL bRet = Trampoline(std::format(L"{}{}", lpNamedPipeName, global::Data::get().envFlagName()).c_str(), nTimeOut))
			{
				return bRet;
			}
		}
		return Trampoline(lpNamedPipeName, nTimeOut);
	}

	template <auto Trampoline>
	HANDLE APIENTRY CreateBoundaryDescriptorA(_In_ LPCSTR Name, _In_ ULONG Flags)
	{
		if (Name)
		{
			return Trampoline(std::format("{}{}", Name, global::Data::get().envFlagNameA()).c_str(), Flags);
		}
		return Trampoline(Name, Flags);
	}

	template <auto Trampoline>
	HANDLE WINAPI CreateBoundaryDescriptorW(_In_ LPCWSTR Name, _In_ ULONG Flags)
	{
		if (Name)
		{
			return Trampoline(std::format(L"{}{}", Name, global::Data::get().envFlagName()).c_str(), Flags);
		}
		return Trampoline(Name, Flags);
	}

	inline PDETOUR_CREATE_PROCESS_ROUTINEW* pCreateProcessTrampolineW{nullptr};

	BOOL inject_dll_to_process(LPPROCESS_INFORMATION lpProcessInformation)
	{
		std::string_view dllFullPath = global::Data::get().dllFullPath();
		LPCSTR sz = dllFullPath.data();

		if (!DetourUpdateProcessWithDll(lpProcessInformation->hProcess, &sz, 1) &&
			!DetourProcessViaHelperW(lpProcessInformation->dwProcessId,
			                         dllFullPath.data(),
			                         *pCreateProcessTrampolineW))
		{
			return FALSE;
		}
		const std::wstring_view rootPath = global::Data::get().rootPath();
		const std::uint32_t rootPathCount = static_cast<std::uint32_t>(rootPath.length());
		const std::uint32_t rootPathSize = rootPathCount * sizeof(wchar_t);
		const std::uint32_t paramsSize = sizeof(DetourInjectParams) + rootPathSize;
		std::vector<std::byte> buffer(paramsSize);
		DetourInjectParams* injectParams = reinterpret_cast<DetourInjectParams*>(buffer.data());
		injectParams->version = global::Data::get().sysVersion();
		injectParams->envFlag = global::Data::get().envFlag();
		injectParams->envIndex = global::Data::get().envIndex();
		injectParams->rootPathCount = rootPathCount;
		memcpy(injectParams->rootPath, rootPath.data(), rootPathSize);
		if (!DetourCopyPayloadToProcess(lpProcessInformation->hProcess, DETOUR_INJECT_PARAMS_GUID, injectParams, paramsSize))
		{
			return FALSE;
		}
		return TRUE;
	}

	template <auto Trampoline>
	BOOL WINAPI CreateProcessA(__in_opt LPCSTR lpApplicationName, __inout_opt LPSTR lpCommandLine,
	                           __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes, __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes,
	                           __in BOOL bInheritHandles, __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment, __in_opt LPCSTR lpCurrentDirectory,
	                           __in LPSTARTUPINFOA lpStartupInfo, __out LPPROCESS_INFORMATION lpProcessInformation)
	{
		const BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
		if (!bOrigSuspended)
		{
			dwCreationFlags |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION backup{};
		if (!lpProcessInformation)
		{
			lpProcessInformation = &backup;
		}

		if (!Trampoline(lpApplicationName, lpCommandLine,
		                lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags,
		                lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
		{
			return FALSE;
		}
		const BOOL bRet = inject_dll_to_process(lpProcessInformation);
		if (bRet)
		{
			if (!bOrigSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
			if (lpProcessInformation == &backup)
			{
				CloseHandle(lpProcessInformation->hProcess);
				CloseHandle(lpProcessInformation->hThread);
			}
		}
		else
		{
			TerminateProcess(lpProcessInformation->hProcess, ~0u);
			CloseHandle(lpProcessInformation->hProcess);
			CloseHandle(lpProcessInformation->hThread);
			if (lpProcessInformation != &backup)
			{
				lpProcessInformation->hProcess = nullptr;
				lpProcessInformation->hThread = nullptr;
				lpProcessInformation->dwProcessId = 0;
				lpProcessInformation->dwThreadId = 0;
			}
		}
		return bRet;
	}

	template <auto Trampoline>
	BOOL WINAPI CreateProcessW(__in_opt LPCWSTR lpApplicationName, __inout_opt LPWSTR lpCommandLine,
	                           __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes, __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, __in BOOL bInheritHandles, __in DWORD dwCreationFlags,
	                           __in_opt LPVOID lpEnvironment, __in_opt LPCWSTR lpCurrentDirectory,
	                           __in LPSTARTUPINFOW lpStartupInfo, __out LPPROCESS_INFORMATION lpProcessInformation)
	{
		const BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
		if (!bOrigSuspended)
		{
			dwCreationFlags |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION backup{};
		if (!lpProcessInformation)
		{
			lpProcessInformation = &backup;
		}

		if (!Trampoline(lpApplicationName, lpCommandLine,
		                lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags,
		                lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation))
		{
			// 因为不知道该怎么优雅的拦截cmd创建管理员进程，因此在这里抹去这个ERROR_ELEVATION_REQUIRED错误
			if (GetLastError() == ERROR_ELEVATION_REQUIRED && global::Data::get().isCmd())
			{
				SetLastError(ERROR_ACCESS_DENIED);
				rpc::default_call_ignore_error(&rpc::ClientDefault::requireElevation, GetCurrentProcessId(), global::Data::get().envFlag(), lpApplicationName ? lpApplicationName : L"");
			}
			return FALSE;
		}
		const BOOL bRet = inject_dll_to_process(lpProcessInformation);
		if (bRet)
		{
			if (!bOrigSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
			if (lpProcessInformation == &backup)
			{
				CloseHandle(lpProcessInformation->hProcess);
				CloseHandle(lpProcessInformation->hThread);
			}
		}
		else
		{
			TerminateProcess(lpProcessInformation->hProcess, ~0u);
			CloseHandle(lpProcessInformation->hProcess);
			CloseHandle(lpProcessInformation->hThread);
			if (lpProcessInformation != &backup)
			{
				lpProcessInformation->hProcess = nullptr;
				lpProcessInformation->hThread = nullptr;
				lpProcessInformation->dwProcessId = 0;
				lpProcessInformation->dwThreadId = 0;
			}
		}
		return bRet;
	}

	template <auto Trampoline>
	UINT WINAPI WinExec(__in LPCSTR lpCmdLine,__in UINT uCmdShow)
	{
		if (!lpCmdLine)
		{
			return 0;
		}

		PROCESS_INFORMATION ProcInfo = {};
		STARTUPINFOA StartupInfo = {sizeof(StartupInfo)};
		StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
		StartupInfo.wShowWindow = uCmdShow;

		std::string strCmd = std::format("{}", lpCmdLine);
		if (!CreateProcessA(nullptr, strCmd.data(), nullptr, nullptr,FALSE,
		                    CREATE_DEFAULT_ERROR_MODE, nullptr, nullptr, &StartupInfo, &ProcInfo))
		{
			return 0;
		}

		if (ProcInfo.hThread)
		{
			CloseHandle(ProcInfo.hThread);
		}

		if (ProcInfo.hProcess)
		{
			CloseHandle(ProcInfo.hProcess);
		}
		return 33;
	}

	bool contains_process_id_in_other_env(DWORD dwProcessId)
	{
		try
		{
			const rpc::ClientDefault c;
			return c.containsProcessIdExclude(dwProcessId, global::Data::get().envFlag());
		}
		catch (...)
		{
		}
		return true;
	}

	template <auto Trampoline>
	HANDLE WINAPI OpenProcess(_In_ DWORD dwDesiredAccess, _In_ BOOL bInheritHandle, _In_ DWORD dwProcessId)
	{
		if (contains_process_id_in_other_env(dwProcessId))
		{
			SetLastError(ERROR_INVALID_PARAMETER);
			return nullptr;
		}
		return Trampoline(dwDesiredAccess, bInheritHandle, dwProcessId);
	}

#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.

#define  FILE_DEVICE_SCSI    0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT 0x0004D008  //  see NTDDSCSI.H for definition

#pragma pack(1)

	typedef struct _MYOUTSMALL
	{
		USHORT wGenConfig;
		USHORT wNumCyls;
		USHORT wReserved;
		USHORT wNumHeads;
		USHORT wBytesPerTrack;
		USHORT wBytesPerSector;
		USHORT wSectorsPerTrack;
		USHORT wVendorUnique[3];
		CHAR sSerialNumber[20];
	} MYOUTSMALL;

	typedef struct _MYOUT
	{
		MYOUTSMALL struMy;
		USHORT wBufferType;
		USHORT wBufferSize;
		USHORT wECCSize;
		CHAR sFirmwareRev[8];
		CHAR sModelNumber[40];
		/* no need
			USHORT  wMoreVendorUnique;
			USHORT  wDoubleWordIO;
			USHORT  wCapabilities;
			USHORT  wReserved1;
			USHORT  wPIOTiming;
			USHORT  wDMATiming;
			USHORT  wBS;
			USHORT  wNumCurrentCyls;
			USHORT  wNumCurrentHeads;
			USHORT  wNumCurrentSectorsPerTrack;
			ULONG   ulCurrentSectorCapacity;
			USHORT  wMultSectorStuff;
			ULONG   ulTotalAddressableSectors;
			USHORT  wSingleWordDMA;
			USHORT  wMultiWordDMA;
			BYTE    bReserved[128];*/
	} MYOUT;

	typedef struct _SRB_IO_CONTROL
	{
		ULONG HeaderLength;
		UCHAR Signature[8];
		ULONG Timeout;
		ULONG ControlCode;
		ULONG ReturnCode;
		ULONG Length;
	} SRB_IO_CONTROL, *PSRB_IO_CONTROL;

#pragma pack()


	template <auto Trampoline>
	std::optional<BOOL> Proc_SMART_RCV_DRIVE_DATA(HANDLE hDevice, DWORD dwIoControlCode,
	                                              LPVOID lpInBuffer, DWORD nInBufferSize,
	                                              LPVOID lpOutBuffer, DWORD nOutBufferSize,
	                                              LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
	{
		if (nInBufferSize < sizeof(SENDCMDINPARAMS) - 1)
		{
			return std::nullopt;
		}
		SENDCMDINPARAMS* pIn = static_cast<SENDCMDINPARAMS*>(lpInBuffer);
		if (IDE_ATAPI_IDENTIFY != pIn->irDriveRegs.bCommandReg && IDE_ATA_IDENTIFY != pIn->irDriveRegs.bCommandReg)
		{
			return std::nullopt;
		}
		if (nOutBufferSize < sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUTSMALL))
		{
			return std::nullopt;
		}
		if (lpOverlapped)
		{
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
		BOOL bRet = Trampoline(hDevice, dwIoControlCode,
		                       lpInBuffer, nInBufferSize,
		                       lpOutBuffer, nOutBufferSize,
		                       lpBytesReturned, nullptr);
		if (!bRet)
		{
			return bRet;
		}
		MYOUT* pOut = reinterpret_cast<MYOUT*>(static_cast<SENDCMDOUTPARAMS*>(lpOutBuffer)->bBuffer);
		std::format_to(pOut->struMy.sSerialNumber, "{}", global::Data::get().envFlagNameA());
		pOut->struMy.sSerialNumber[sizeof(pOut->struMy.sSerialNumber) - 1] = '\0';
		if (nOutBufferSize >= sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUT))
		{
			std::format_to(pOut->sModelNumber, "{}", global::Data::get().envFlagNameA());
			pOut->sModelNumber[sizeof(pOut->sModelNumber) - 1] = '\0';
		}
		return bRet;
	}

	template <auto Trampoline>
	std::optional<BOOL> Proc_IOCTL_SCSI_MINIPORT(HANDLE hDevice, DWORD dwIoControlCode,
	                                             LPVOID lpInBuffer, DWORD nInBufferSize,
	                                             LPVOID lpOutBuffer, DWORD nOutBufferSize,
	                                             LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
	{
		if (nInBufferSize < sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1)
		{
			return std::nullopt;
		}
		SRB_IO_CONTROL* p = static_cast<SRB_IO_CONTROL*>(lpInBuffer);
		SENDCMDINPARAMS* pin = reinterpret_cast<SENDCMDINPARAMS*>(static_cast<char*>(lpInBuffer) + sizeof(SRB_IO_CONTROL));
		if (sizeof(SRB_IO_CONTROL) != p->HeaderLength
			&& IOCTL_SCSI_MINIPORT_IDENTIFY != p->ControlCode
			&& std::string_view{reinterpret_cast<char*>(p->Signature), sizeof(p->Signature)} != std::string_view{"SCSIDISK"}
			&& IDE_ATA_IDENTIFY != pin->irDriveRegs.bCommandReg
			&& IDE_ATAPI_IDENTIFY != pin->irDriveRegs.bCommandReg)
		{
			return std::nullopt;
		}
		if (nOutBufferSize < sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUTSMALL))
		{
			return std::nullopt;
		}
		if (lpOverlapped)
		{
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
		BOOL bRet = Trampoline(hDevice, dwIoControlCode,
		                       lpInBuffer, nInBufferSize,
		                       lpOutBuffer, nOutBufferSize,
		                       lpBytesReturned, nullptr);
		if (!bRet)
		{
			return bRet;
		}
		SENDCMDOUTPARAMS* pOutParams = reinterpret_cast<SENDCMDOUTPARAMS*>(static_cast<char*>(lpOutBuffer) + sizeof(SRB_IO_CONTROL));
		MYOUT* pOut = reinterpret_cast<MYOUT*>(pOutParams->bBuffer);
		std::format_to(pOut->struMy.sSerialNumber, "{}", global::Data::get().envFlagNameA());
		pOut->struMy.sSerialNumber[sizeof(pOut->struMy.sSerialNumber) - 1] = '\0';
		if (nOutBufferSize >= sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUT))
		{
			std::format_to(pOut->sModelNumber, "{}", global::Data::get().envFlagNameA());
			pOut->sModelNumber[sizeof(pOut->sModelNumber) - 1] = '\0';
		}
		return bRet;
	}

	template <auto Trampoline>
	std::optional<BOOL> Proc_IOCTL_STORAGE_QUERY_PROPERTY(HANDLE hDevice, DWORD dwIoControlCode,
	                                                      LPVOID lpInBuffer, DWORD nInBufferSize,
	                                                      LPVOID lpOutBuffer, DWORD nOutBufferSize,
	                                                      LPDWORD lpBytesReturned, LPOVERLAPPED lpOverlapped)
	{
		if (nInBufferSize < sizeof(STORAGE_PROPERTY_QUERY))
		{
			return std::nullopt;
		}
		STORAGE_PROPERTY_QUERY* query = static_cast<STORAGE_PROPERTY_QUERY*>(lpInBuffer);
		if (query->QueryType == PropertyExistsQuery)
		{
			return std::nullopt;
		}
		if (query->PropertyId != StorageDeviceProperty)
		{
			return std::nullopt;
		}
		if (nOutBufferSize < sizeof(STORAGE_DESCRIPTOR_HEADER))
		{
			return std::nullopt;
		}
		if (lpOverlapped)
		{
			SetLastError(ERROR_ACCESS_DENIED);
			return FALSE;
		}
		BOOL bRet = Trampoline(hDevice, dwIoControlCode,
		                       lpInBuffer, nInBufferSize,
		                       lpOutBuffer, nOutBufferSize,
		                       lpBytesReturned, nullptr);
		if (!bRet)
		{
			return bRet;
		}
		PSTORAGE_DEVICE_DESCRIPTOR sdn = static_cast<PSTORAGE_DEVICE_DESCRIPTOR>(lpOutBuffer);
		const std::string_view flagName = global::Data::get().envFlagNameA();
		constexpr DWORD iMinLen = sizeof(STORAGE_DEVICE_DESCRIPTOR);
		sdn->Size = std::min(sdn->Size, static_cast<DWORD>(flagName.length() + iMinLen));

		if (nOutBufferSize <= iMinLen)
		{
			return bRet;
		}
		DWORD bytesReturned = std::min(sdn->Size, nOutBufferSize);
		if (lpBytesReturned)
		{
			*lpBytesReturned = bytesReturned;
		}
		sdn->RawPropertiesLength = bytesReturned - iMinLen;
		sdn->VendorIdOffset = 0;
		sdn->ProductIdOffset = 0;
		sdn->ProductRevisionOffset = 0;
		sdn->SerialNumberOffset = iMinLen;

		BYTE* pData = reinterpret_cast<BYTE*>(sdn) + iMinLen;
		memcpy(pData, flagName.data(), sdn->RawPropertiesLength);
		pData[sdn->RawPropertiesLength - 1] = 0;
		return bRet;
	}

	template <auto Trampoline>
	BOOL WINAPI DeviceIoControl(__in HANDLE hDevice, __in DWORD dwIoControlCode, __in_bcount_opt(nInBufferSize)
	                            LPVOID lpInBuffer, __in DWORD nInBufferSize,
	                            LPVOID lpOutBuffer, __in DWORD nOutBufferSize,
	                            __out_opt LPDWORD lpBytesReturned, __inout_opt LPOVERLAPPED lpOverlapped)
	{
		std::optional<BOOL> result = std::nullopt;

		if (lpInBuffer && lpOutBuffer)
		{
			if (SMART_RCV_DRIVE_DATA == dwIoControlCode)
			{
				result = Proc_SMART_RCV_DRIVE_DATA<Trampoline>(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
			}
			else if (IOCTL_SCSI_MINIPORT == dwIoControlCode)
			{
				result = Proc_IOCTL_SCSI_MINIPORT<Trampoline>(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
			}
			else if (IOCTL_STORAGE_QUERY_PROPERTY == dwIoControlCode)
			{
				result = Proc_IOCTL_STORAGE_QUERY_PROPERTY<Trampoline>(hDevice, dwIoControlCode, lpInBuffer, nInBufferSize, lpOutBuffer, nOutBufferSize, lpBytesReturned, lpOverlapped);
			}
		}

		if (result.has_value())
		{
			return result.value();
		}
		return Trampoline(hDevice, dwIoControlCode,
		                  lpInBuffer, nInBufferSize,
		                  lpOutBuffer, nOutBufferSize,
		                  lpBytesReturned, lpOverlapped);
	}

	void hook_kernel32()
	{
		constexpr auto KERNEL32_LIB_NAME = utils::make_literal_name<L"kernel32.dll">();
		sys_info::SysDllMapHelper kernel32Mapped = sys_info::get_kernel32_mapped();
		void* kernel32MappedAddress = kernel32Mapped.memAddress();

#define CREATE_HOOK_BY_NAME(name) \
	create_hook_by_func_type<KERNEL32_LIB_NAME, utils::make_literal_name<#name>(), decltype(name<nullptr>)>().setHookFromGetter([&](auto trampolineConst) \
	{ \
		return HookInfo{&name<trampolineConst.value>, kernel32MappedAddress}; \
	})

		CREATE_HOOK_BY_NAME(WaitNamedPipeA);
		CREATE_HOOK_BY_NAME(WaitNamedPipeW);
		CREATE_HOOK_BY_NAME(CreateBoundaryDescriptorA);
		CREATE_HOOK_BY_NAME(CreateBoundaryDescriptorW);
		CREATE_HOOK_BY_NAME(CreateProcessA);
		pCreateProcessTrampolineW = std::addressof(CREATE_HOOK_BY_NAME(CreateProcessW).funcAddress);
		CREATE_HOOK_BY_NAME(WinExec);
		CREATE_HOOK_BY_NAME(OpenProcess);
		CREATE_HOOK_BY_NAME(DeviceIoControl);
	}
}
