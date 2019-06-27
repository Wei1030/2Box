#include "StdAfx.h"
#include "Kernel32Hook.h"
#include "InitialData.h"
#include "TrampolineFunc.h"
#include "detours.h"

extern void* g_pClient;

TRAMPOLINE(BOOL(WINAPI*)(LPCSTR,DWORD),WaitNamedPipeA);
TRAMPOLINE(BOOL(WINAPI*)(LPCWSTR,DWORD),WaitNamedPipeW);

TRAMPOLINE(BOOL(WINAPI*)( LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL, 
		   DWORD,LPVOID,LPCSTR,LPSTARTUPINFOA,LPPROCESS_INFORMATION),
		   CreateProcessA);
TRAMPOLINE(BOOL(WINAPI*)(LPCWSTR,LPWSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,
		   DWORD,LPVOID,LPCWSTR,LPSTARTUPINFOW,LPPROCESS_INFORMATION),
		   CreateProcessW);

TRAMPOLINE(BOOL(WINAPI*)(HANDLE,LPCSTR,LPSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL, 
		   DWORD,LPVOID,LPCSTR,LPSTARTUPINFOA,LPPROCESS_INFORMATION),
		   CreateProcessAsUserA);
TRAMPOLINE(BOOL(WINAPI*)(HANDLE,LPCWSTR,LPWSTR,LPSECURITY_ATTRIBUTES,LPSECURITY_ATTRIBUTES,BOOL,
		   DWORD,LPVOID,LPCWSTR,LPSTARTUPINFOW,LPPROCESS_INFORMATION),
		   CreateProcessAsUserW);

TRAMPOLINE(UINT(WINAPI*)(LPCSTR,UINT), WinExec);

TRAMPOLINE(BOOL(WINAPI*)(HANDLE,HANDLE), AssignProcessToJobObject);

TRAMPOLINE(BOOL(WINAPI*)(HANDLE,DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD,LPOVERLAPPED), DeviceIoControl);

CKernel32Hook::CKernel32Hook(void)
: CBaseHook()
{
}

CKernel32Hook::~CKernel32Hook(void)
{
}

BOOL CKernel32Hook::Init(CDbghelpWrapper* pHelper)
{
	BOOL bValRet = FALSE;

	do 
	{		
		CBaseHook::InitFakeFile(L"kernel32");				
		
		HMODULE hMod = LoadLibraryW(L"kernel32.dll");
		if (NULL == hMod)
		{
			break;
		}

		HOOK(CKernel32Hook,hMod,WaitNamedPipeA,pHelper);
		HOOK(CKernel32Hook,hMod,WaitNamedPipeW,pHelper);
		HOOK_FROM_FILE_EAT(CKernel32Hook,hMod,CreateProcessA,pHelper);
		HOOK_FROM_FILE_EAT(CKernel32Hook,hMod,CreateProcessW,pHelper);
		HOOK(CKernel32Hook,hMod,CreateProcessAsUserA,pHelper);
		HOOK(CKernel32Hook,hMod,CreateProcessAsUserW,pHelper);
		HOOK_FROM_FILE_EAT(CKernel32Hook,hMod,WinExec,pHelper);
		HOOK(CKernel32Hook,hMod,AssignProcessToJobObject,pHelper);
		HOOK(CKernel32Hook,hMod,DeviceIoControl,pHelper);

		bValRet = TRUE;

	} while (0);

	return bValRet;
}

BOOL WINAPI CKernel32Hook::WaitNamedPipeA(__in LPCSTR lpNamedPipeName,__in DWORD nTimeOut)
{
	if (lpNamedPipeName)
	{
		//xp上WaitNamedPipeA内部会直接调用WaitNamedPipeW，这里也直接进行W版的调用，统一在W版修改PipeName;
		wchar_t szNameW[MAX_PATH];
		int wChs = MultiByteToWideChar(CP_ACP,0,lpNamedPipeName,-1,szNameW,MAX_PATH);
		if (wChs)
		{
			return ::WaitNamedPipeW(szNameW,nTimeOut);
		}
	}
	return TrueWaitNamedPipeA.Call()(lpNamedPipeName,nTimeOut);
}

BOOL WINAPI CKernel32Hook::WaitNamedPipeW(__in LPCWSTR lpNamedPipeName, __in DWORD nTimeOut)
{
	if (lpNamedPipeName)
	{
		std::wstring strNewName = lpNamedPipeName;				
		strNewName += g_pData->GetNewNameW();		
		return TrueWaitNamedPipeW.Call()(strNewName.c_str(),nTimeOut);
	}
	return TrueWaitNamedPipeW.Call()(lpNamedPipeName,nTimeOut);
}

BOOL WINAPI CKernel32Hook::CreateProcessA(__in_opt LPCSTR lpApplicationName, __inout_opt LPSTR lpCommandLine,
										  __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes, 
										  __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, 
										  __in BOOL bInheritHandles, __in DWORD dwCreationFlags, 
										  __in_opt LPVOID lpEnvironment, __in_opt LPCSTR lpCurrentDirectory,
										  __in LPSTARTUPINFOA lpStartupInfo, __out LPPROCESS_INFORMATION lpProcessInformation)
{
	dwCreationFlags &= ~CREATE_BREAKAWAY_FROM_JOB; //不允许脱离job;

	BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
	if (FALSE == bOrigSuspended)
	{
		dwCreationFlags |= CREATE_SUSPENDED;
	}

	BOOL bEnvInitResult = FALSE;
	void* lpNewEnv = NULL;
	std::wstring wStr;
	std::string aStr;

	if (lpEnvironment)
	{
		if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT)
		{
			bEnvInitResult = g_pData->ProcUnknownEnvStringsW(lpEnvironment,wStr);
			lpNewEnv = (void*)wStr.c_str();
		}
		else
		{
			bEnvInitResult = g_pData->ProcUnknownEnvStringsA(lpEnvironment,aStr);
			lpNewEnv = (void*)aStr.c_str();
		}
	}
	else
	{
		dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
		LPWCH pW = GetEnvironmentStringsW();
		if (NULL == pW)
		{
			return FALSE;
		}

		bEnvInitResult = g_pData->ProcUnknownEnvStringsW(pW,wStr);
		lpNewEnv = (void*)wStr.c_str();

		FreeEnvironmentStringsW(pW);
	}

	if (FALSE == bEnvInitResult)
	{
		return FALSE;
	}

	PROCESS_INFORMATION backup;
	if (lpProcessInformation == NULL) {
		lpProcessInformation = &backup;
		ZeroMemory(&backup, sizeof(backup));
	}

	BOOL bRet = DetourCreateProcessWithDllExA(lpApplicationName,lpCommandLine,lpProcessAttributes,
		lpThreadAttributes,bInheritHandles,dwCreationFlags,lpNewEnv,lpCurrentDirectory,
		lpStartupInfo,lpProcessInformation,g_pData->GetDllPathA(),TrueCreateProcessA.Call());

	if (bRet)
	{
		if (0 == SRPC_Call(g_pClient,"2Box.NewProcess",&lpProcessInformation->dwProcessId,sizeof(DWORD),NULL,NULL))
		{
			if (FALSE == bOrigSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
		}
		else
		{
			TerminateProcess(lpProcessInformation->hProcess,0);
		}
	}

	if (lpProcessInformation == &backup) {
		CloseHandle(lpProcessInformation->hProcess);
		CloseHandle(lpProcessInformation->hThread);
	}
	return bRet;
}

BOOL WINAPI CKernel32Hook::CreateProcessW(__in_opt LPCWSTR lpApplicationName, __inout_opt LPWSTR lpCommandLine, 
										  __in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes, 
										  __in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, 
										  __in BOOL bInheritHandles, __in DWORD dwCreationFlags, 
										  __in_opt LPVOID lpEnvironment, __in_opt LPCWSTR lpCurrentDirectory, 
										  __in LPSTARTUPINFOW lpStartupInfo, __out LPPROCESS_INFORMATION lpProcessInformation)
{
	dwCreationFlags &= ~CREATE_BREAKAWAY_FROM_JOB; //不允许脱离job;

	BOOL bOrigSuspended = dwCreationFlags & CREATE_SUSPENDED;
	if (FALSE == bOrigSuspended)
	{
		dwCreationFlags |= CREATE_SUSPENDED;
	}

	BOOL bEnvInitResult = FALSE;
	void* lpNewEnv = NULL;
	std::wstring wStr;
	std::string aStr;

	if (lpEnvironment)
	{
		if (dwCreationFlags & CREATE_UNICODE_ENVIRONMENT)
		{
			bEnvInitResult = g_pData->ProcUnknownEnvStringsW(lpEnvironment,wStr);
			lpNewEnv = (void*)wStr.c_str();
		}
		else
		{
			bEnvInitResult = g_pData->ProcUnknownEnvStringsA(lpEnvironment,aStr);
			lpNewEnv = (void*)aStr.c_str();
		}
	}
	else
	{
		dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
		LPWCH pW = GetEnvironmentStringsW();
		if (NULL == pW)
		{
			return FALSE;
		}

		bEnvInitResult = g_pData->ProcUnknownEnvStringsW(pW,wStr);
		lpNewEnv = (void*)wStr.c_str();

		FreeEnvironmentStringsW(pW);
	}

	if (FALSE == bEnvInitResult)
	{
		return FALSE;
	}

	PROCESS_INFORMATION backup;
	if (lpProcessInformation == NULL) {
		lpProcessInformation = &backup;
		ZeroMemory(&backup, sizeof(backup));
	}

	BOOL bRet = DetourCreateProcessWithDllExW(lpApplicationName,lpCommandLine,lpProcessAttributes,
		lpThreadAttributes,bInheritHandles,dwCreationFlags,lpNewEnv,lpCurrentDirectory,
		lpStartupInfo,lpProcessInformation,g_pData->GetDllPathA(),TrueCreateProcessW.Call());

	if (bRet)
	{
		if (0 == SRPC_Call(g_pClient,"2Box.NewProcess",&lpProcessInformation->dwProcessId,sizeof(DWORD),NULL,NULL))
		{
			if (FALSE == bOrigSuspended)
			{
				ResumeThread(lpProcessInformation->hThread);
			}
		}
		else
		{
			TerminateProcess(lpProcessInformation->hProcess,0);
		}
	}

	if (lpProcessInformation == &backup) {
		CloseHandle(lpProcessInformation->hProcess);
		CloseHandle(lpProcessInformation->hThread);
	}
	return bRet;
}

BOOL WINAPI CKernel32Hook::CreateProcessAsUserA(__in_opt HANDLE hToken, __in_opt LPCSTR lpApplicationName, 
												__inout_opt LPSTR lpCommandLine, 
												__in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes, 
												__in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, 
												__in BOOL bInheritHandles, __in DWORD dwCreationFlags, 
												__in_opt LPVOID lpEnvironment, __in_opt LPCSTR lpCurrentDirectory, 
												__in LPSTARTUPINFOA lpStartupInfo, 
												__out LPPROCESS_INFORMATION lpProcessInformation)
{
	return CreateProcessA(lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,
		dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
}

BOOL WINAPI CKernel32Hook::CreateProcessAsUserW(__in_opt HANDLE hToken, __in_opt LPCWSTR lpApplicationName,
												__inout_opt LPWSTR lpCommandLine,
												__in_opt LPSECURITY_ATTRIBUTES lpProcessAttributes,
												__in_opt LPSECURITY_ATTRIBUTES lpThreadAttributes, 
												__in BOOL bInheritHandles, __in DWORD dwCreationFlags, 
												__in_opt LPVOID lpEnvironment, __in_opt LPCWSTR lpCurrentDirectory, 
												__in LPSTARTUPINFOW lpStartupInfo,
												__out LPPROCESS_INFORMATION lpProcessInformation)
{
	return CreateProcessW(lpApplicationName,lpCommandLine,lpProcessAttributes,lpThreadAttributes,bInheritHandles,
		dwCreationFlags,lpEnvironment,lpCurrentDirectory,lpStartupInfo,lpProcessInformation);
}

UINT WINAPI CKernel32Hook::WinExec(__in LPCSTR lpCmdLine,__in UINT uCmdShow)
{
	if (NULL == lpCmdLine)
	{
		return 0;
	}

	PROCESS_INFORMATION ProcInfo	= {0};  
	STARTUPINFOA StartupInfo		= {0};
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = uCmdShow;

	char szCmd[MAX_PATH] = {0};
	strcpy_s(szCmd,lpCmdLine);
	BOOL bRet = CreateProcessA(NULL,szCmd,NULL,NULL,FALSE,
		CREATE_DEFAULT_ERROR_MODE,NULL,NULL,&StartupInfo,&ProcInfo);

	if (FALSE == bRet)
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

BOOL WINAPI CKernel32Hook::AssignProcessToJobObject(HANDLE hJob, HANDLE hProcess)
{
	return TRUE;
}

#define  IDE_ATAPI_IDENTIFY  0xA1  //  Returns ID sector for ATAPI.
#define  IDE_ATA_IDENTIFY    0xEC  //  Returns ID sector for ATA.

#define  FILE_DEVICE_SCSI    0x0000001b
#define  IOCTL_SCSI_MINIPORT_IDENTIFY  ((FILE_DEVICE_SCSI << 16) + 0x0501)
#define  IOCTL_SCSI_MINIPORT 0x0004D008  //  see NTDDSCSI.H for definition

#pragma pack(1)

typedef struct _MYOUTSMALL
{
	USHORT  wGenConfig;
	USHORT  wNumCyls;
	USHORT  wReserved;
	USHORT  wNumHeads;
	USHORT  wBytesPerTrack;
	USHORT  wBytesPerSector;
	USHORT  wSectorsPerTrack;
	USHORT  wVendorUnique[3];
	CHAR    sSerialNumber[20];
}MYOUTSMALL;

typedef struct _MYOUT
{
	MYOUTSMALL struMy;
	USHORT  wBufferType;
	USHORT  wBufferSize;
	USHORT  wECCSize;
	CHAR    sFirmwareRev[8];
	CHAR    sModelNumber[40];
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

BOOL WINAPI CKernel32Hook::DeviceIoControl(__in HANDLE hDevice, 
										   __in DWORD dwIoControlCode, 
										   __in_bcount_opt(nInBufferSize) LPVOID lpInBuffer, 
										   __in DWORD nInBufferSize, 
										   __out_bcount_part_opt(nOutBufferSize, *lpBytesReturned) LPVOID lpOutBuffer, 
										   __in DWORD nOutBufferSize, 
										   __out_opt LPDWORD lpBytesReturned,
										   __inout_opt LPOVERLAPPED lpOverlapped)
{
	BOOL bProcessed = FALSE;

	do 
	{
		if (NULL == lpInBuffer || NULL == lpOutBuffer )
		{
			break;
		}

		if (SMART_RCV_DRIVE_DATA == dwIoControlCode )
		{
			if ( nInBufferSize < (sizeof(SENDCMDINPARAMS)-1))
			{
				break;
			}

			SENDCMDINPARAMS* pIn = (SENDCMDINPARAMS*)lpInBuffer;

			if (IDE_ATAPI_IDENTIFY != pIn->irDriveRegs.bCommandReg
				&& IDE_ATA_IDENTIFY != pIn->irDriveRegs.bCommandReg)
			{
				break;
			}

			if ( nOutBufferSize < (sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUTSMALL)) )
			{
				break;
			}

			TrueDeviceIoControl.Call()(hDevice,dwIoControlCode,
				lpInBuffer,nInBufferSize,
				lpOutBuffer,nOutBufferSize,
				lpBytesReturned,NULL);

			MYOUT *pOut = (MYOUT*) ((PSENDCMDOUTPARAMS) lpOutBuffer) -> bBuffer;
			memcpy(pOut->struMy.sSerialNumber,g_pData->GetNewNameA(),20);
			if ( nOutBufferSize >= (sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUT)) )
			{
				memcpy(pOut->sModelNumber,g_pData->GetNewNameA(),32);
			}

			bProcessed = TRUE;
			break;
		}

		if (IOCTL_SCSI_MINIPORT == dwIoControlCode)
		{
			if (nInBufferSize < sizeof (SRB_IO_CONTROL) + sizeof (SENDCMDINPARAMS) - 1)
			{
				break;
			}
			SRB_IO_CONTROL *p = (SRB_IO_CONTROL *) lpInBuffer;
			SENDCMDINPARAMS *pin =(SENDCMDINPARAMS *) ((char*)lpInBuffer + sizeof (SRB_IO_CONTROL));

			if (sizeof (SRB_IO_CONTROL) != p->HeaderLength
				&& IOCTL_SCSI_MINIPORT_IDENTIFY != p->ControlCode
				&& _stricmp((char*)p->Signature,"SCSIDISK") != 0
				&& IDE_ATA_IDENTIFY != pin->irDriveRegs.bCommandReg
				&& IDE_ATAPI_IDENTIFY != pin->irDriveRegs.bCommandReg)
			{
				break;
			}

			if ( nOutBufferSize < (sizeof (SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUTSMALL)) )
			{
				break;
			}

			TrueDeviceIoControl.Call()(hDevice,dwIoControlCode,
				lpInBuffer,nInBufferSize,
				lpOutBuffer,nOutBufferSize,
				lpBytesReturned,NULL);

			SENDCMDOUTPARAMS *pOutParams = (SENDCMDOUTPARAMS *)((char*)lpOutBuffer + sizeof (SRB_IO_CONTROL));
			MYOUT *pOut = (MYOUT*) (pOutParams-> bBuffer);
			memcpy(pOut->struMy.sSerialNumber,g_pData->GetNewNameA(),20);
			if ( nOutBufferSize >= (sizeof (SRB_IO_CONTROL) + sizeof(SENDCMDOUTPARAMS) - 1 + sizeof(MYOUT)) )
			{
				memcpy(pOut->sModelNumber,g_pData->GetNewNameA(),32);
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

			TrueDeviceIoControl.Call()(hDevice,dwIoControlCode,
				lpInBuffer,nInBufferSize,
				lpOutBuffer,nOutBufferSize,
				lpBytesReturned,NULL);

			DWORD iNewLen = (DWORD)strlen(g_pData->GetNewNameA());
			DWORD iMinLen = sizeof(STORAGE_DEVICE_DESCRIPTOR);
			PSTORAGE_DEVICE_DESCRIPTOR sdn = (PSTORAGE_DEVICE_DESCRIPTOR)lpOutBuffer;
			sdn->Size = iMinLen + iNewLen;
			bProcessed = TRUE;

			if (nOutBufferSize <= iMinLen)
			{
				break;
			}

			if (nOutBufferSize < sdn->Size)
			{
				iNewLen = nOutBufferSize - iMinLen - 1;
				if ( 0 == iNewLen)
				{
					break;
				}
			}

			sdn->RawPropertiesLength = iNewLen;

			sdn->VendorIdOffset			= iMinLen;
			sdn->ProductIdOffset		= iMinLen;
			sdn->ProductRevisionOffset	= iMinLen;
			sdn->SerialNumberOffset		= iMinLen;

			BYTE* pData = ((BYTE*)sdn) + iMinLen;

			memcpy(pData,g_pData->GetNewNameA(),iNewLen);
			pData[iNewLen] = 0;

			break;
		}

	} while (0);

	if (FALSE == bProcessed)
	{
		return TrueDeviceIoControl.Call()(hDevice,dwIoControlCode,
			lpInBuffer,nInBufferSize,
			lpOutBuffer,nOutBufferSize,
			lpBytesReturned,lpOverlapped);
	}

	return bProcessed;
}
