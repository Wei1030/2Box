#include "StdAfx.h"
#include "NtdllHook.h"
#include "InitialData.h"
#include "TrampolineFunc.h"

#include <algorithm>

//////////////////////////////////////////////////////////////////////////
//Event
TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,int,BOOLEAN),NtCreateEvent);

TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES),NtOpenEvent);

//////////////////////////////////////////////////////////////////////////
//Mutant
TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,BOOLEAN),NtCreateMutant);

TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES),NtOpenMutant);

//////////////////////////////////////////////////////////////////////////
//Section
TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PLARGE_INTEGER,ULONG,ULONG,HANDLE),NtCreateSection);

TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES),NtOpenSection);

//////////////////////////////////////////////////////////////////////////
//Semaphore
TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,LONG,LONG),NtCreateSemaphore);

TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES),NtOpenSemaphore);


//////////////////////////////////////////////////////////////////////////
//Timer
TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,int),NtCreateTimer);

TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES),NtOpenTimer);

//////////////////////////////////////////////////////////////////////////
//Job
TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES),NtCreateJobObject);

TRAMPOLINE(NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES),NtOpenJobObject);


//////////////////////////////////////////////////////////////////////////
//NamedPipe
TRAMPOLINE(NTSTATUS(NTAPI*)( PHANDLE,ULONG,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK, 
								 ULONG,ULONG,ULONG,ULONG,ULONG,ULONG, 
								 ULONG,ULONG,ULONG,PLARGE_INTEGER), NtCreateNamedPipeFile);

TRAMPOLINE(NTSTATUS(NTAPI*)( PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES, 
								 PIO_STATUS_BLOCK,PLARGE_INTEGER, 
								 ULONG,ULONG,ULONG,ULONG,PVOID,ULONG),NtCreateFile);

//////////////////////////////////////////////////////////////////////////
//other
TRAMPOLINE(NTSTATUS(NTAPI*)( PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES, 
								 PIO_STATUS_BLOCK,ULONG,ULONG),NtOpenFile);

TRAMPOLINE(NTSTATUS(NTAPI*)(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG),NtQuerySystemInformation);


CNtdllHook::CNtdllHook(void)
{
}

CNtdllHook::~CNtdllHook(void)
{
}

BOOL CNtdllHook::Init()
{
	CBaseHook::InitFile(L"ntdll");

	BOOL bValRet = FALSE;

	do 
	{
		HMODULE hMod = GetModuleHandleW(L"ntdll.dll");
		if (NULL == hMod)
		{
			break;
		}

		HOOK(CNtdllHook,hMod,NtCreateEvent);
		HOOK(CNtdllHook,hMod,NtOpenEvent);

		HOOK(CNtdllHook,hMod,NtCreateMutant);
		HOOK(CNtdllHook,hMod,NtOpenMutant);

		HOOK(CNtdllHook,hMod,NtCreateSection);
		HOOK(CNtdllHook,hMod,NtOpenSection);

		HOOK(CNtdllHook,hMod,NtCreateSemaphore);
		HOOK(CNtdllHook,hMod,NtOpenSemaphore);

		HOOK(CNtdllHook,hMod,NtCreateTimer);
		HOOK(CNtdllHook,hMod,NtOpenTimer);

		HOOK(CNtdllHook,hMod,NtCreateJobObject);
		HOOK(CNtdllHook,hMod,NtOpenJobObject);

		HOOK(CNtdllHook,hMod,NtCreateNamedPipeFile);
		HOOK(CNtdllHook,hMod,NtCreateFile);

		HOOK(CNtdllHook,hMod,NtOpenFile);
		HOOK(CNtdllHook,hMod,NtQuerySystemInformation);

		bValRet = TRUE;

	} while (0);

	CBaseHook::UninitFile();
	return bValRet;
}
//////////////////////////////////////////////////////////////////////////
//event
NTSTATUS NTAPI CNtdllHook::NtCreateEvent(OUT PHANDLE EventHandle, IN ACCESS_MASK DesiredAccess, 
										 IN POBJECT_ATTRIBUTES ObjectAttributes, IN int EventType, IN BOOLEAN InitialState)
{
	CHANGE_OBJ_NAME(TrueNtCreateEvent.Call()(EventHandle,
		DesiredAccess,ObjectAttributes,EventType,InitialState));
}

NTSTATUS NTAPI CNtdllHook::NtOpenEvent(OUT PHANDLE EventHandle, IN ACCESS_MASK DesiredAccess, 
									   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenEvent.Call()(EventHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//mutant
NTSTATUS NTAPI CNtdllHook::NtCreateMutant(OUT PHANDLE MutantHandle, IN ACCESS_MASK DesiredAccess, 
										  IN POBJECT_ATTRIBUTES ObjectAttributes, IN BOOLEAN InitialOwner)
{
	CHANGE_OBJ_NAME(TrueNtCreateMutant.Call()(MutantHandle,
		DesiredAccess,ObjectAttributes,InitialOwner));
}

NTSTATUS NTAPI CNtdllHook::NtOpenMutant(OUT PHANDLE MutantHandle, IN ACCESS_MASK DesiredAccess,
										IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenMutant.Call()(MutantHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//section
NTSTATUS NTAPI CNtdllHook::NtCreateSection(OUT PHANDLE SectionHandle, IN ACCESS_MASK DesiredAccess, 
										   IN POBJECT_ATTRIBUTES ObjectAttributes, IN PLARGE_INTEGER SectionSize 
										   OPTIONAL, IN ULONG Protect, IN ULONG Attributes, IN HANDLE FileHandle)
{
	CHANGE_OBJ_NAME(TrueNtCreateSection.Call()(SectionHandle,
		DesiredAccess,ObjectAttributes,SectionSize,	Protect,Attributes,FileHandle));
}

NTSTATUS NTAPI CNtdllHook::NtOpenSection(OUT PHANDLE SectionHandle, IN ACCESS_MASK DesiredAccess, 
										 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenSection.Call()(SectionHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//semaphore
NTSTATUS NTAPI CNtdllHook::NtCreateSemaphore(OUT PHANDLE SemaphoreHandle, IN ACCESS_MASK DesiredAccess, 
											 IN POBJECT_ATTRIBUTES ObjectAttributes, IN LONG InitialCount, 
											 IN LONG MaximumCount)
{
	CHANGE_OBJ_NAME(TrueNtCreateSemaphore.Call()(SemaphoreHandle,
		DesiredAccess,ObjectAttributes,InitialCount,MaximumCount));
}

NTSTATUS NTAPI CNtdllHook::NtOpenSemaphore(OUT PHANDLE SemaphoreHandle, IN ACCESS_MASK DesiredAccess, 
										   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenSemaphore.Call()(SemaphoreHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//timer
NTSTATUS NTAPI CNtdllHook::NtCreateTimer(OUT PHANDLE TimerHandle, IN ACCESS_MASK DesiredAccess, 
										 IN POBJECT_ATTRIBUTES ObjectAttributes, IN int TimerType)
{
	CHANGE_OBJ_NAME(TrueNtCreateTimer.Call()(TimerHandle,
		DesiredAccess,ObjectAttributes,TimerType));
}

NTSTATUS NTAPI CNtdllHook::NtOpenTimer(OUT PHANDLE TimerHandle, IN ACCESS_MASK DesiredAccess, 
									   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenTimer.Call()(TimerHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//job
NTSTATUS NTAPI CNtdllHook::NtCreateJobObject(OUT PHANDLE JobHandle, IN ACCESS_MASK DesiredAccess, 
											 IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	CHANGE_OBJ_NAME(TrueNtCreateJobObject.Call()(JobHandle,
		DesiredAccess,ObjectAttributes));
}

NTSTATUS NTAPI CNtdllHook::NtOpenJobObject(OUT PHANDLE JobHandle, IN ACCESS_MASK DesiredAccess, 
										   IN POBJECT_ATTRIBUTES ObjectAttributes)
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenJobObject.Call()(JobHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//named pipe
NTSTATUS NTAPI CNtdllHook::NtCreateNamedPipeFile(_Out_ PHANDLE FileHandle, _In_ ULONG DesiredAccess, 
												 _In_ POBJECT_ATTRIBUTES ObjectAttributes, 
												 _Out_ PIO_STATUS_BLOCK IoStatusBlock, _In_ ULONG ShareAccess, 
												 _In_ ULONG CreateDisposition, _In_ ULONG CreateOptions, 
												 _In_ ULONG NamedPipeType, _In_ ULONG ReadMode, _In_ ULONG CompletionMode, 
												 _In_ ULONG MaximumInstances, _In_ ULONG InboundQuota, 
												 _In_ ULONG OutboundQuota, _In_opt_ PLARGE_INTEGER DefaultTimeout)
{
	CHANGE_OBJ_NAME(TrueNtCreateNamedPipeFile.Call()(FileHandle,
		DesiredAccess,ObjectAttributes,IoStatusBlock,ShareAccess,CreateDisposition,
		CreateOptions,NamedPipeType,ReadMode,CompletionMode,MaximumInstances,
		InboundQuota,OutboundQuota,DefaultTimeout));
}

NTSTATUS NTAPI CNtdllHook::NtCreateFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, 
										IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PIO_STATUS_BLOCK IoStatusBlock, 
										IN PLARGE_INTEGER AllocationSize OPTIONAL, IN ULONG FileAttributes, 
										IN ULONG ShareAccess, IN ULONG CreateDisposition, IN ULONG CreateOptions,
										IN PVOID EaBuffer OPTIONAL, IN ULONG EaLength)
{
	NTSTATUS ret = -1;	

	do 
	{
		if (NULL == ObjectAttributes
			|| NULL == ObjectAttributes->ObjectName
			|| NULL == ObjectAttributes->ObjectName->Buffer
			|| 0 == ObjectAttributes->ObjectName->Length)
		{
			break;
		}

		std::wstring strCmpName(ObjectAttributes->ObjectName->Buffer,
			ObjectAttributes->ObjectName->Length/sizeof(wchar_t));

		//管道;
		if (strCmpName.find(L"\\??\\pipe\\") !=  std::wstring::npos)
		{/* \??\pipe\ */			
			PUNICODE_STRING pOldName =  ObjectAttributes->ObjectName;
			UNICODE_STRING struW = {0};

			strCmpName += g_pData->GetNewNameW();						
			struW.Buffer = (wchar_t*)strCmpName.c_str();					
			struW.Length = struW.MaximumLength = (USHORT)strCmpName.length()*sizeof(wchar_t);
			ObjectAttributes->ObjectName = &struW;						
			ret = TrueNtCreateFile.Call()(FileHandle,
				DesiredAccess,ObjectAttributes,IoStatusBlock,AllocationSize,
				FileAttributes,ShareAccess,CreateDisposition,CreateOptions,
				EaBuffer,EaLength);											
			ObjectAttributes->ObjectName = pOldName;						
			break;					
		}

// 		if (strCmpName.find(L"\\??\\PhysicalDrive") != std::wstring::npos)
// 		{//\??\PhysicalDrive
// 			return (NTSTATUS)0xC0000022L;
// 		}
// 
// 		if (strCmpName.find(L"\\??\\Scsi") != std::wstring::npos)
// 		{
// 			return (NTSTATUS)0xC0000022L;
// 		}

		//文件;
		std::transform(strCmpName.begin(), strCmpName.end(), strCmpName.begin(), tolower);
		size_t nPos = strCmpName.find('\\',2);
		if (-1 == nPos)
		{
			break;
		}

		nPos +=1;
		std::wstring strFileName = strCmpName.substr(nPos);
		//wprintf_s(L"create:%s,find:%s\r\n",strCmpName.c_str(),strFileName.c_str());

		if (FALSE == g_pData->IsAllowedAccess(strFileName))
		{
			wprintf_s(L"create not access:%s\r\n",strFileName.c_str());
			return (NTSTATUS)0xC0000022L;
		}
	} while (0);

	if ( ret < 0)
	{
		ret = TrueNtCreateFile.Call()(FileHandle,
			DesiredAccess,ObjectAttributes,IoStatusBlock,AllocationSize,
			FileAttributes,ShareAccess,CreateDisposition,CreateOptions,
			EaBuffer,EaLength);		
	}

	return ret;	
}

//////////////////////////////////////////////////////////////////////////
//other
NTSTATUS NTAPI CNtdllHook::NtOpenFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess, 
									  IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PIO_STATUS_BLOCK IoStatusBlock, 
									  IN ULONG ShareAccess, IN ULONG OpenOptions)
{
	NTSTATUS ret = -1;	

	do 
	{
		if (NULL == ObjectAttributes
			|| NULL == ObjectAttributes->ObjectName
			|| NULL == ObjectAttributes->ObjectName->Buffer
			|| 0 == ObjectAttributes->ObjectName->Length)
		{
			break;
		}

		std::wstring strCmpName(ObjectAttributes->ObjectName->Buffer,
			ObjectAttributes->ObjectName->Length/sizeof(wchar_t));
		std::transform(strCmpName.begin(), strCmpName.end(), strCmpName.begin(), tolower);
		size_t nPos = strCmpName.find('\\',2);
		if (-1 == nPos)
		{
			break;
		}
		nPos +=1;
		std::wstring strFileName = strCmpName.substr(nPos);
		//wprintf_s(L"open:%s,find:%s\r\n",strCmpName.c_str(),strFileName.c_str());

		if (FALSE == g_pData->IsAllowedAccess(strFileName))
		{
			wprintf_s(L"open not access:%s\r\n",strFileName.c_str());
			return (NTSTATUS)0xC0000022L;
		}

	} while (0);

	if ( ret < 0)
	{
		ret = TrueNtOpenFile.Call()(FileHandle,
			DesiredAccess,ObjectAttributes,IoStatusBlock,ShareAccess,OpenOptions);
	}
	return ret;	
}

BOOL GetAllProcessInJob(std::set<ULONG_PTR>& setPid)
{
	BOOL bGetProcessIdRet	= FALSE;
	int iProcessNum			= 20;
	DWORD cb				= 0;
	PJOBOBJECT_BASIC_PROCESS_ID_LIST pjobpil = NULL;

	do 
	{
		if (pjobpil)
		{
			free(pjobpil);
		}
		cb = sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + (sizeof(ULONG_PTR) * (iProcessNum - 1));
		pjobpil = (PJOBOBJECT_BASIC_PROCESS_ID_LIST) malloc(cb);			
		if (NULL == pjobpil)
		{
			break;
		}
		pjobpil->NumberOfProcessIdsInList = iProcessNum;

		if (FALSE == ::QueryInformationJobObject(NULL, JobObjectBasicProcessIdList, 
			pjobpil, cb, NULL))
		{
			break;
		}

		if (pjobpil->NumberOfProcessIdsInList < pjobpil->NumberOfAssignedProcesses)
		{
			iProcessNum = pjobpil->NumberOfAssignedProcesses * 2;
			continue;
		}

		for (DWORD i = 0;i< pjobpil->NumberOfProcessIdsInList; ++i)
		{
			setPid.insert(pjobpil->ProcessIdList[i]);
		}

		bGetProcessIdRet = TRUE;
	} while (FALSE == bGetProcessIdRet);

	if (pjobpil)
	{
		free(pjobpil);
	}

	return bGetProcessIdRet;
}

NTSTATUS NTAPI CNtdllHook::NtQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass, 
													IN OUT PVOID SystemInformation, IN ULONG SystemInformationLength,
													OUT PULONG ReturnLength OPTIONAL)
{
	NTSTATUS ret = TrueNtQuerySystemInformation.Call()(SystemInformationClass,SystemInformation,
		SystemInformationLength,ReturnLength);

	if (ret < 0 || NULL == SystemInformation)
	{
		return ret;
	}

	if ( SystemInformationClass == SystemProcessInformation )
	{
		std::set<ULONG_PTR> set_proc_to_show;
		if (FALSE == GetAllProcessInJob(set_proc_to_show))
		{
			set_proc_to_show.insert(GetCurrentProcessId());
		}			

		PSYSTEM_PROCESS_INFORMATION pIndex = (PSYSTEM_PROCESS_INFORMATION)SystemInformation;
		PSYSTEM_PROCESS_INFORMATION pShow  = pIndex;

		do 
		{
			if (pIndex->UniqueProcessId != NULL
				&& set_proc_to_show.find((ULONG_PTR)pIndex->UniqueProcessId) == set_proc_to_show.end()
				/*&& pIndex->UniqueProcessId != (HANDLE)GetCurrentProcessId()*/)
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
				pIndex = (PSYSTEM_PROCESS_INFORMATION)((char*)pIndex + pIndex->NextEntryOffset);
			}
			else
			{
				pIndex = NULL;
			}
		} while (pIndex);
	}
	return ret;
}
