#include "stdafx.h"
#include "Hook_Ntdll.h"
#include "InitialData.h"
#include "TrampolineFunc.h"

#include <set>
#include <algorithm>


//////////////////////////////////////////////////////////////////////////
//Event
CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,int,BOOLEAN)>
TrueNtCreateEvent("NtCreateEvent",&Hook_NtCreateEvent);

CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES)>
TrueNtOpenEvent("NtOpenEvent",&Hook_NtOpenEvent);

NTSTATUS NTAPI Hook_NtCreateEvent( OUT PHANDLE EventHandle,
								  IN ACCESS_MASK DesiredAccess, 
								  IN POBJECT_ATTRIBUTES ObjectAttributes, 
								  IN int EventType, 
								  IN BOOLEAN InitialState )
{
	CHANGE_OBJ_NAME(TrueNtCreateEvent.Call()(EventHandle,
		DesiredAccess,ObjectAttributes,EventType,InitialState));
}

NTSTATUS NTAPI Hook_NtOpenEvent( OUT PHANDLE EventHandle, 
								IN ACCESS_MASK DesiredAccess, 
								IN POBJECT_ATTRIBUTES ObjectAttributes )
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenEvent.Call()(EventHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//Mutant
CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,BOOLEAN)>
TrueNtCreateMutant("NtCreateMutant",&Hook_NtCreateMutant);

CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES)>
TrueNtOpenMutant("NtOpenMutant",&Hook_NtOpenMutant);

NTSTATUS NTAPI Hook_NtCreateMutant( OUT PHANDLE MutantHandle, 
								   IN ACCESS_MASK DesiredAccess, 
								   IN POBJECT_ATTRIBUTES ObjectAttributes, 
								   IN BOOLEAN InitialOwner )
{
	CHANGE_OBJ_NAME(TrueNtCreateMutant.Call()(MutantHandle,
		DesiredAccess,ObjectAttributes,InitialOwner));
}

NTSTATUS NTAPI Hook_NtOpenMutant( OUT PHANDLE MutantHandle, 
								 IN ACCESS_MASK DesiredAccess, 
								 IN POBJECT_ATTRIBUTES ObjectAttributes )
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenMutant.Call()(MutantHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//Section
CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,PLARGE_INTEGER,ULONG,ULONG,HANDLE)>
TrueNtCreateSection("NtCreateSection",&Hook_NtCreateSection);

CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES)>
TrueNtOpenSection("NtOpenSection",&Hook_NtOpenSection);

NTSTATUS NTAPI Hook_NtCreateSection( OUT PHANDLE SectionHandle, 
									IN ACCESS_MASK DesiredAccess, 
									IN POBJECT_ATTRIBUTES ObjectAttributes, 
									IN PLARGE_INTEGER SectionSize OPTIONAL,
									IN ULONG Protect, 
									IN ULONG Attributes, 
									IN HANDLE FileHandle )
{	
	CHANGE_OBJ_NAME(TrueNtCreateSection.Call()(SectionHandle,
		DesiredAccess,ObjectAttributes,SectionSize,	Protect,Attributes,FileHandle));
}

NTSTATUS NTAPI Hook_NtOpenSection( OUT PHANDLE SectionHandle,
								  IN ACCESS_MASK DesiredAccess,
								  IN POBJECT_ATTRIBUTES ObjectAttributes )
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenSection.Call()(SectionHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//Semaphore
CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,LONG,LONG)>
TrueNtCreateSemaphore("NtCreateSemaphore",&Hook_NtCreateSemaphore);

CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES)>
TrueNtOpenSemaphore("NtOpenSemaphore",&Hook_NtOpenSemaphore);

NTSTATUS NTAPI Hook_NtCreateSemaphore( OUT PHANDLE SemaphoreHandle,
									  IN ACCESS_MASK DesiredAccess, 
									  IN POBJECT_ATTRIBUTES ObjectAttributes, 
									  IN LONG InitialCount, 
									  IN LONG MaximumCount )
{
	CHANGE_OBJ_NAME(TrueNtCreateSemaphore.Call()(SemaphoreHandle,
		DesiredAccess,ObjectAttributes,InitialCount,MaximumCount));
}

NTSTATUS NTAPI Hook_NtOpenSemaphore( OUT PHANDLE SemaphoreHandle, 
									IN ACCESS_MASK DesiredAccess, 
									IN POBJECT_ATTRIBUTES ObjectAttributes )
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenSemaphore.Call()(SemaphoreHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//Timer
CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES,int)>
TrueNtCreateTimer("NtCreateTimer",&Hook_NtCreateTimer);

CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES)>
TrueNtOpenTimer("NtOpenTimer",&Hook_NtOpenTimer);

NTSTATUS NTAPI Hook_NtCreateTimer( OUT PHANDLE TimerHandle, 
								  IN ACCESS_MASK DesiredAccess,
								  IN POBJECT_ATTRIBUTES ObjectAttributes, 
								  IN int TimerType )
{
	CHANGE_OBJ_NAME(TrueNtCreateTimer.Call()(TimerHandle,
		DesiredAccess,ObjectAttributes,TimerType));
}

NTSTATUS NTAPI Hook_NtOpenTimer( OUT PHANDLE TimerHandle,
								IN ACCESS_MASK DesiredAccess, 
								IN POBJECT_ATTRIBUTES ObjectAttributes )
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenTimer.Call()(TimerHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//Job
CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES)>
TrueNtCreateJobObject("NtCreateJobObject",&Hook_NtCreateJobObject);

CTrampolineFunc<NTSTATUS(NTAPI*)(PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES)>
TrueNtOpenJobObject("NtOpenJobObject",&Hook_NtOpenJobObject);

NTSTATUS NTAPI Hook_NtCreateJobObject( OUT PHANDLE JobHandle, 
									  IN ACCESS_MASK DesiredAccess, 
									  IN POBJECT_ATTRIBUTES ObjectAttributes )
{
	CHANGE_OBJ_NAME(TrueNtCreateJobObject.Call()(JobHandle,
		DesiredAccess,ObjectAttributes));
}

NTSTATUS NTAPI Hook_NtOpenJobObject( OUT PHANDLE JobHandle, 
									IN ACCESS_MASK DesiredAccess, 
									IN POBJECT_ATTRIBUTES ObjectAttributes )
{
	TRY_TO_CHANGE_OBJ_NAME(TrueNtOpenJobObject.Call()(JobHandle,
		DesiredAccess,ObjectAttributes));
}

//////////////////////////////////////////////////////////////////////////
//NamedPipe
CTrampolineFunc<NTSTATUS(NTAPI*)( PHANDLE,ULONG,POBJECT_ATTRIBUTES,PIO_STATUS_BLOCK, 
								 ULONG,ULONG,ULONG,ULONG,ULONG,ULONG, 
								 ULONG,ULONG,ULONG,PLARGE_INTEGER)>
TrueNtCreateNamedPipeFile("NtCreateNamedPipeFile",&Hook_NtCreateNamedPipeFile);

CTrampolineFunc<NTSTATUS(NTAPI*)( PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES, 
								 PIO_STATUS_BLOCK,PLARGE_INTEGER, 
								 ULONG,ULONG,ULONG,ULONG,PVOID,ULONG)>
TrueNtCreateFile("NtCreateFile",&Hook_NtCreateFile);

NTSTATUS NTAPI Hook_NtCreateNamedPipeFile( _Out_ PHANDLE FileHandle, _In_ ULONG DesiredAccess, 
										  _In_ POBJECT_ATTRIBUTES ObjectAttributes, _Out_ PIO_STATUS_BLOCK IoStatusBlock, 
										  _In_ ULONG ShareAccess, _In_ ULONG CreateDisposition, _In_ ULONG CreateOptions,
										  _In_ ULONG NamedPipeType, _In_ ULONG ReadMode, _In_ ULONG CompletionMode, 
										  _In_ ULONG MaximumInstances, _In_ ULONG InboundQuota, _In_ ULONG OutboundQuota,
										  _In_opt_ PLARGE_INTEGER DefaultTimeout )
{
	CHANGE_OBJ_NAME(TrueNtCreateNamedPipeFile.Call()(FileHandle,
		DesiredAccess,ObjectAttributes,IoStatusBlock,ShareAccess,CreateDisposition,
		CreateOptions,NamedPipeType,ReadMode,CompletionMode,MaximumInstances,
		InboundQuota,OutboundQuota,DefaultTimeout));
}

NTSTATUS NTAPI Hook_NtCreateFile( OUT PHANDLE FileHandle, 
								 IN ACCESS_MASK DesiredAccess, 
								 IN POBJECT_ATTRIBUTES ObjectAttributes, 
								 OUT PIO_STATUS_BLOCK IoStatusBlock, 
								 IN PLARGE_INTEGER AllocationSize OPTIONAL, 
								 IN ULONG FileAttributes, 
								 IN ULONG ShareAccess, 
								 IN ULONG CreateDisposition, 
								 IN ULONG CreateOptions, 
								 IN PVOID EaBuffer OPTIONAL, 
								 IN ULONG EaLength )
{
	NTSTATUS ret = -1;	

	if (ObjectAttributes											
		&& ObjectAttributes->ObjectName								
		&& ObjectAttributes->ObjectName->Buffer
		&& ObjectAttributes->ObjectName->Length)
	{
		std::wstring strCmpName(ObjectAttributes->ObjectName->Buffer,
			ObjectAttributes->ObjectName->Length/sizeof(wchar_t));
		//管道;
		if (strCmpName.find(L"\\??\\pipe\\") !=  std::wstring::npos)
		{/* \??\pipe\ */	
			//if (strCmpName.find(L"{BE6D2570-E5E5-40f0-9A53-CBDDE9C422BD}") == std::wstring::npos)
			{
				PUNICODE_STRING pOldName =  ObjectAttributes->ObjectName;
				UNICODE_STRING struW = {0};

				strCmpName += g_pData->GetNewNameW();						
				struW.Buffer = (wchar_t*)strCmpName.c_str();					
				struW.Length = struW.MaximumLength = strCmpName.length()*sizeof(wchar_t);
				ObjectAttributes->ObjectName = &struW;						
				ret = TrueNtCreateFile.Call()(FileHandle,
					DesiredAccess,ObjectAttributes,IoStatusBlock,AllocationSize,
					FileAttributes,ShareAccess,CreateDisposition,CreateOptions,
					EaBuffer,EaLength);											
				ObjectAttributes->ObjectName = pOldName;						
				//return ret;			
			}			
		}		
// 		else if (strCmpName.find(L"\\??\\PhysicalDrive") != std::wstring::npos)
// 		{//\??\PhysicalDrive
// 			return (NTSTATUS)0xC0000022L;
// 		}
// 		else if (strCmpName.find(L"\\??\\Scsi") != std::wstring::npos)
// 		{
// 			return (NTSTATUS)0xC0000022L;
// 		}
		else
		{//文件;
			std::transform(strCmpName.begin(), strCmpName.end(), strCmpName.begin(), tolower);
			if (strCmpName.find(L"ntdll") != std::wstring::npos)
			{
				return (NTSTATUS)0xC0000022L;
			}
		}
	}

	if ( ret < 0)
	{
		ret = TrueNtCreateFile.Call()(FileHandle,
			DesiredAccess,ObjectAttributes,IoStatusBlock,AllocationSize,
			FileAttributes,ShareAccess,CreateDisposition,CreateOptions,
			EaBuffer,EaLength);		
	}

	return ret;
}  

CTrampolineFunc<NTSTATUS(NTAPI*)( PHANDLE,ACCESS_MASK,POBJECT_ATTRIBUTES, 
								 PIO_STATUS_BLOCK,ULONG,ULONG)>
TrueNtOpenFile("NtOpenFile",&Hook_NtOpenFile);

NTSTATUS NTAPI Hook_NtOpenFile(OUT PHANDLE FileHandle, IN ACCESS_MASK DesiredAccess,
							   IN POBJECT_ATTRIBUTES ObjectAttributes, OUT PIO_STATUS_BLOCK IoStatusBlock,
							   IN ULONG ShareAccess, IN ULONG OpenOptions)
{
	if (ObjectAttributes											
		&& ObjectAttributes->ObjectName								
		&& ObjectAttributes->ObjectName->Buffer
		&& ObjectAttributes->ObjectName->Length)
	{
		std::wstring strCmpName(ObjectAttributes->ObjectName->Buffer,
			ObjectAttributes->ObjectName->Length/sizeof(wchar_t));
		std::transform(strCmpName.begin(), strCmpName.end(), strCmpName.begin(), tolower);

		if (strCmpName.find(L"ntdll") != std::wstring::npos)
		{
			return (NTSTATUS)0xC0000022L;
		}
	}

	return TrueNtOpenFile.Call()(FileHandle,
		DesiredAccess,ObjectAttributes,IoStatusBlock,ShareAccess,OpenOptions);
}

BOOL GetAllProcessInJob(std::set<ULONG_PTR>& setPid)
{
	BOOL bGetProcessIdRet	= FALSE;
	int iProcessNum			= 20;
	ULONG_PTR cb			= 0;
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

CTrampolineFunc<NTSTATUS(NTAPI*)(SYSTEM_INFORMATION_CLASS,PVOID,ULONG,PULONG)>
TrueNtQuerySystemInformation("NtQuerySystemInformation",&Hook_NtQuerySystemInformation);

NTSTATUS NTAPI Hook_NtQuerySystemInformation( IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
											 IN OUT PVOID SystemInformation, 
											 IN ULONG SystemInformationLength,
											 OUT PULONG ReturnLength OPTIONAL )
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
