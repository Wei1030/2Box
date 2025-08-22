export module Hook:Kernel32;

import "sys_defs.h";
import :Core;
import std;
import GlobalData;
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

	template <auto Trampoline>
	BOOL WINAPI CreateProcessA(__in_opt LPCSTR lpApplicationName, __inout_opt LPSTR lpCommandLine,
	                           __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes, __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes,
	                           __in BOOL bInheritHandles, __in DWORD dwCreationFlags, __in_opt LPVOID lpEnvironment, __in_opt LPCSTR lpCurrentDirectory,
	                           __in LPSTARTUPINFOA lpStartupInfo, __out LPPROCESS_INFORMATION lpProcessInformation)
	{
		BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
		if (!bOrigSuspended)
		{
			dwCreationFlags |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION backup{};
		if (!lpProcessInformation)
		{
			lpProcessInformation = &backup;
		}
		BOOL bRet = DetourCreateProcessWithDllExA(lpApplicationName, lpCommandLine,
		                                          lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags,
		                                          lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation,
		                                          global::Data::get().dllFullPath().data(), Trampoline.funcAddress);
		if (!bRet)
		{
			return bRet;
		}

		try
		{
			const std::wstring_view rootPath = global::Data::get().rootPath();
			const std::uint32_t rootPathCount = static_cast<std::uint32_t>(rootPath.length());
			const std::uint32_t rootPathSize = rootPathCount * sizeof(wchar_t);
			const std::uint32_t paramsSize = sizeof(DetourInjectParams) + rootPathSize;
			std::vector<std::byte> buffer(paramsSize);
			DetourInjectParams* injectParams = reinterpret_cast<DetourInjectParams*>(buffer.data());
			injectParams->version = pe::g_os_version;
			injectParams->envFlag = global::Data::get().envFlag();
			injectParams->envIndex = global::Data::get().envIndex();
			injectParams->rootPathCount = rootPathCount;
			memcpy(injectParams->rootPath, rootPath.data(), rootPathSize);
			if (!DetourCopyPayloadToProcess(lpProcessInformation->hProcess, DETOUR_INJECT_PARAMS_GUID, injectParams, paramsSize))
			{
				throw std::runtime_error(std::format("copy payload failed, error code: {}", GetLastError()));
			}
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
		catch (...)
		{
			bRet = FALSE;
			TerminateProcess(lpProcessInformation->hProcess, 0);

			if (lpProcessInformation)
			{
				CloseHandle(lpProcessInformation->hProcess);
				CloseHandle(lpProcessInformation->hThread);

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
		BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
		if (!bOrigSuspended)
		{
			dwCreationFlags |= CREATE_SUSPENDED;
		}

		PROCESS_INFORMATION backup{};
		if (!lpProcessInformation)
		{
			lpProcessInformation = &backup;
		}
		BOOL bRet = DetourCreateProcessWithDllExW(lpApplicationName, lpCommandLine,
		                                          lpProcessAttributes, lpThreadAttributes, bInheritHandles, dwCreationFlags,
		                                          lpEnvironment, lpCurrentDirectory, lpStartupInfo, lpProcessInformation,
		                                          global::Data::get().dllFullPath().data(), Trampoline.funcAddress);
		if (!bRet)
		{
			return bRet;
		}
		try
		{
			const std::wstring_view rootPath = global::Data::get().rootPath();
			const std::uint32_t rootPathCount = static_cast<std::uint32_t>(rootPath.length());
			const std::uint32_t rootPathSize = rootPathCount * sizeof(wchar_t);
			const std::uint32_t paramsSize = sizeof(DetourInjectParams) + rootPathSize;
			std::vector<std::byte> buffer(paramsSize);
			DetourInjectParams* injectParams = reinterpret_cast<DetourInjectParams*>(buffer.data());
			injectParams->version = pe::g_os_version;
			injectParams->envFlag = global::Data::get().envFlag();
			injectParams->envIndex = global::Data::get().envIndex();
			injectParams->rootPathCount = rootPathCount;
			memcpy(injectParams->rootPath, rootPath.data(), rootPathSize);
			if (!DetourCopyPayloadToProcess(lpProcessInformation->hProcess, DETOUR_INJECT_PARAMS_GUID, injectParams, paramsSize))
			{
				throw std::runtime_error(std::format("copy payload failed, error code: {}", GetLastError()));
			}
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
		catch (...)
		{
			bRet = FALSE;
			TerminateProcess(lpProcessInformation->hProcess, 0);

			if (lpProcessInformation)
			{
				CloseHandle(lpProcessInformation->hProcess);
				CloseHandle(lpProcessInformation->hThread);

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
	BOOL WINAPI DeviceIoControl(__in HANDLE hDevice, __in DWORD dwIoControlCode, __in_bcount_opt(nInBufferSize)
	                            LPVOID lpInBuffer, __in DWORD nInBufferSize,
	                            LPVOID lpOutBuffer, __in DWORD nOutBufferSize,
	                            __out_opt LPDWORD lpBytesReturned, __inout_opt LPOVERLAPPED lpOverlapped)
	{
		BOOL bProcessed = FALSE;

		do
		{
			if (!lpInBuffer || !lpOutBuffer)
			{
				break;
			}

			if (SMART_RCV_DRIVE_DATA == dwIoControlCode)
			{
				if (nInBufferSize < sizeof(SENDCMDINPARAMS) - 1)
				{
					break;
				}

				SENDCMDINPARAMS* pIn = static_cast<SENDCMDINPARAMS*>(lpInBuffer);

				if (IDE_ATAPI_IDENTIFY != pIn->irDriveRegs.bCommandReg
					&& IDE_ATA_IDENTIFY != pIn->irDriveRegs.bCommandReg)
				{
					break;
				}

				if (nOutBufferSize < sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUTSMALL))
				{
					break;
				}
				Trampoline(hDevice, dwIoControlCode,
				           lpInBuffer, nInBufferSize,
				           lpOutBuffer, nOutBufferSize,
				           lpBytesReturned, nullptr);

				MYOUT* pOut = reinterpret_cast<MYOUT*>(static_cast<PSENDCMDOUTPARAMS>(lpOutBuffer)->bBuffer);
				std::format_to(pOut->struMy.sSerialNumber, "{}", global::Data::get().envFlagNameA());
				pOut->struMy.sSerialNumber[sizeof(pOut->struMy.sSerialNumber) - 1] = '\0';
				if (nOutBufferSize >= sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUT))
				{
					std::format_to(pOut->sModelNumber, "{}", global::Data::get().envFlagNameA());
					pOut->sModelNumber[sizeof(pOut->sModelNumber) - 1] = '\0';
				}
				bProcessed = TRUE;
				break;
			}

			if (IOCTL_SCSI_MINIPORT == dwIoControlCode)
			{
				if (nInBufferSize < sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDINPARAMS) - 1)
				{
					break;
				}
				SRB_IO_CONTROL* p = static_cast<SRB_IO_CONTROL*>(lpInBuffer);
				SENDCMDINPARAMS* pin = reinterpret_cast<SENDCMDINPARAMS*>(static_cast<char*>(lpInBuffer) + sizeof(SRB_IO_CONTROL));
				if (sizeof(SRB_IO_CONTROL) != p->HeaderLength
					&& IOCTL_SCSI_MINIPORT_IDENTIFY != p->ControlCode
					&& std::string_view{reinterpret_cast<char*>(p->Signature), sizeof(p->Signature)} != std::string_view{"SCSIDISK"}
					&& IDE_ATA_IDENTIFY != pin->irDriveRegs.bCommandReg
					&& IDE_ATAPI_IDENTIFY != pin->irDriveRegs.bCommandReg)
				{
					break;
				}
				if (nOutBufferSize < (sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUTSMALL)))
				{
					break;
				}
				Trampoline(hDevice, dwIoControlCode,
				           lpInBuffer, nInBufferSize,
				           lpOutBuffer, nOutBufferSize,
				           lpBytesReturned, nullptr);
				SENDCMDOUTPARAMS* pOutParams = reinterpret_cast<SENDCMDOUTPARAMS*>(static_cast<char*>(lpOutBuffer) + sizeof(SRB_IO_CONTROL));
				MYOUT* pOut = reinterpret_cast<MYOUT*>(pOutParams->bBuffer);
				std::format_to(pOut->struMy.sSerialNumber, "{}", global::Data::get().envFlagNameA());
				pOut->struMy.sSerialNumber[sizeof(pOut->struMy.sSerialNumber) - 1] = '\0';
				if (nOutBufferSize >= (sizeof(SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUT)))
				{
					std::format_to(pOut->sModelNumber, "{}", global::Data::get().envFlagNameA());
					pOut->sModelNumber[sizeof(pOut->sModelNumber) - 1] = '\0';
				}
				bProcessed = TRUE;
				break;
			}

			if (IOCTL_STORAGE_QUERY_PROPERTY == dwIoControlCode)
			{
				if (nInBufferSize < sizeof(STORAGE_PROPERTY_QUERY))
				{
					break;
				}

				if (nOutBufferSize < 8)
				{
					break;
				}

				Trampoline(hDevice, dwIoControlCode,
				           lpInBuffer, nInBufferSize,
				           lpOutBuffer, nOutBufferSize,
				           lpBytesReturned, nullptr);
				const std::string_view newName = global::Data::get().envFlagNameA();
				constexpr DWORD iMinLen = sizeof(STORAGE_DEVICE_DESCRIPTOR);
				DWORD iNewLen = static_cast<DWORD>(newName.length());
				PSTORAGE_DEVICE_DESCRIPTOR sdn = static_cast<PSTORAGE_DEVICE_DESCRIPTOR>(lpOutBuffer);
				sdn->Size = iMinLen + iNewLen;
				bProcessed = TRUE;

				if (nOutBufferSize <= iMinLen)
				{
					break;
				}

				if (nOutBufferSize < sdn->Size)
				{
					iNewLen = nOutBufferSize - iMinLen - 1;
					if (0 == iNewLen)
					{
						break;
					}
				}

				sdn->RawPropertiesLength = iNewLen;
				sdn->VendorIdOffset = iMinLen;
				sdn->ProductIdOffset = iMinLen;
				sdn->ProductRevisionOffset = iMinLen;
				sdn->SerialNumberOffset = iMinLen;

				BYTE* pData = reinterpret_cast<BYTE*>(sdn) + iMinLen;

				memcpy(pData, newName.data(), iNewLen);
				pData[iNewLen] = 0;
				break;
			}
		}
		while (false);

		if (!bProcessed)
		{
			return Trampoline(hDevice, dwIoControlCode,
			                  lpInBuffer, nInBufferSize,
			                  lpOutBuffer, nOutBufferSize,
			                  lpBytesReturned, lpOverlapped);
		}
		return bProcessed;
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
		CREATE_HOOK_BY_NAME(CreateProcessW);
		CREATE_HOOK_BY_NAME(WinExec);
		CREATE_HOOK_BY_NAME(DeviceIoControl);
	}
}
