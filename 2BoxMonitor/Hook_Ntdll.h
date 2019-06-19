#pragma once
#include <Winternl.h>

//////////////////////////////////////////////////////////////////////////
//Event
NTSTATUS NTAPI  Hook_NtCreateEvent(OUT PHANDLE EventHandle,
								   IN ACCESS_MASK DesiredAccess,
								   IN POBJECT_ATTRIBUTES ObjectAttributes,
								   IN int EventType,
								   IN BOOLEAN InitialState);

NTSTATUS NTAPI Hook_NtOpenEvent(OUT PHANDLE EventHandle,
								IN ACCESS_MASK DesiredAccess,
								IN POBJECT_ATTRIBUTES ObjectAttributes);

//////////////////////////////////////////////////////////////////////////
//Mutant
NTSTATUS NTAPI  Hook_NtCreateMutant(OUT PHANDLE MutantHandle,
									IN ACCESS_MASK DesiredAccess,
									IN POBJECT_ATTRIBUTES ObjectAttributes,
									IN BOOLEAN InitialOwner);

NTSTATUS NTAPI Hook_NtOpenMutant(OUT PHANDLE MutantHandle,
								 IN ACCESS_MASK DesiredAccess,
								 IN POBJECT_ATTRIBUTES ObjectAttributes);

//////////////////////////////////////////////////////////////////////////
//Section
NTSTATUS NTAPI Hook_NtCreateSection(OUT PHANDLE SectionHandle,
									IN ACCESS_MASK DesiredAccess,
									IN POBJECT_ATTRIBUTES ObjectAttributes,
									IN PLARGE_INTEGER SectionSize OPTIONAL,
									IN ULONG Protect,
									IN ULONG Attributes,
									IN HANDLE FileHandle
									);

NTSTATUS NTAPI Hook_NtOpenSection(OUT PHANDLE SectionHandle,
								  IN ACCESS_MASK DesiredAccess,
								  IN POBJECT_ATTRIBUTES ObjectAttributes
								  );

//////////////////////////////////////////////////////////////////////////
//Semaphore
NTSTATUS NTAPI Hook_NtCreateSemaphore( OUT PHANDLE SemaphoreHandle,
									  IN ACCESS_MASK DesiredAccess,
									  IN POBJECT_ATTRIBUTES ObjectAttributes,
									  IN LONG InitialCount,
									  IN LONG MaximumCount);

NTSTATUS NTAPI Hook_NtOpenSemaphore(OUT PHANDLE SemaphoreHandle,
									IN ACCESS_MASK DesiredAccess,
									IN POBJECT_ATTRIBUTES ObjectAttributes
									);

//////////////////////////////////////////////////////////////////////////
//Timer
NTSTATUS NTAPI  Hook_NtCreateTimer(OUT PHANDLE TimerHandle,
								   IN ACCESS_MASK DesiredAccess,
								   IN POBJECT_ATTRIBUTES ObjectAttributes,
								   IN int TimerType);

NTSTATUS NTAPI Hook_NtOpenTimer(OUT PHANDLE TimerHandle,
								IN ACCESS_MASK DesiredAccess,
								IN POBJECT_ATTRIBUTES ObjectAttributes);

//////////////////////////////////////////////////////////////////////////
//Job
NTSTATUS NTAPI Hook_NtCreateJobObject(OUT PHANDLE JobHandle,
									  IN ACCESS_MASK DesiredAccess,
									  IN POBJECT_ATTRIBUTES ObjectAttributes);

NTSTATUS NTAPI Hook_NtOpenJobObject(OUT PHANDLE JobHandle,
									IN ACCESS_MASK DesiredAccess,
									IN POBJECT_ATTRIBUTES ObjectAttributes);

//////////////////////////////////////////////////////////////////////////
//NamedPipe
NTSTATUS NTAPI  Hook_NtCreateNamedPipeFile(	_Out_ PHANDLE 	FileHandle,
										   _In_ ULONG 	DesiredAccess,
										   _In_ POBJECT_ATTRIBUTES 	ObjectAttributes,
										   _Out_ PIO_STATUS_BLOCK 	IoStatusBlock,
										   _In_ ULONG 	ShareAccess,
										   _In_ ULONG 	CreateDisposition,
										   _In_ ULONG 	CreateOptions,
										   _In_ ULONG 	NamedPipeType,
										   _In_ ULONG 	ReadMode,
										   _In_ ULONG 	CompletionMode,
										   _In_ ULONG 	MaximumInstances,
										   _In_ ULONG 	InboundQuota,
										   _In_ ULONG 	OutboundQuota,
										   _In_opt_ PLARGE_INTEGER 	DefaultTimeout );

NTSTATUS NTAPI  Hook_NtCreateFile(OUT PHANDLE FileHandle,
								  IN ACCESS_MASK DesiredAccess,
								  IN POBJECT_ATTRIBUTES ObjectAttributes,
								  OUT PIO_STATUS_BLOCK IoStatusBlock,
								  IN PLARGE_INTEGER AllocationSize OPTIONAL,
								  IN ULONG FileAttributes,
								  IN ULONG ShareAccess,
								  IN ULONG CreateDisposition,
								  IN ULONG CreateOptions,
								  IN PVOID EaBuffer OPTIONAL,
								  IN ULONG EaLength);

NTSTATUS NTAPI Hook_NtOpenFile(OUT PHANDLE FileHandle,
							   IN ACCESS_MASK DesiredAccess,
							   IN POBJECT_ATTRIBUTES ObjectAttributes,
							   OUT PIO_STATUS_BLOCK IoStatusBlock,
							   IN ULONG ShareAccess,
							   IN ULONG OpenOptions);

//////////////////////////////////////////////////////////////////////////
//reg key
// NTSTATUS NTAPI  Hook_NtCreateKey(OUT PHANDLE KeyHandle,
// 								 IN ACCESS_MASK DesiredAccess,
// 								 IN POBJECT_ATTRIBUTES ObjectAttributes,
// 								 IN ULONG TitleIndex,
// 								 IN PUNICODE_STRING Class OPTIONAL,
// 								 IN ULONG CreateOptions,
// 								 OUT PULONG Disposition OPTIONAL);

// NTSTATUS NTAPI Hook_NtOpenKey(OUT PHANDLE KeyHandle,
// 							  IN ACCESS_MASK DesiredAccess,
// 							  IN POBJECT_ATTRIBUTES ObjectAttributes);

// NTSTATUS NTAPI Hook_NtRenameKey(IN HANDLE KeyHandle,IN PUNICODE_STRING ReplacementName);

// NTSTATUS NTAPI Hook_NtSetValueKey(IN HANDLE KeyHandle,
// 								  IN PUNICODE_STRING ValueName,
// 								  IN ULONG TitleIndex,
// 								  IN ULONG Type,
// 								  IN PVOID Data,
// 								  IN ULONG DataSize );

// NTSTATUS NTAPI Hook_NtAlpcConnectPort(	_Out_ PHANDLE 	PortHandle,
//												 _In_ PUNICODE_STRING 	PortName,
//												 _In_opt_ POBJECT_ATTRIBUTES 	ObjectAttributes,
//												 _In_opt_ void*	PortAttributes,
//												 _In_ ULONG 	Flags,
//												 _In_opt_ PSID 	RequiredServerSid,
//												 void* 	ConnectionMessage,
//												 _Inout_opt_ PULONG 	BufferLength,
//												 _Inout_opt_ void* 	OutMessageAttributes,
//												 _Inout_opt_ void* 	InMessageAttributes,
//												 _In_opt_ PLARGE_INTEGER 	Timeout);

//////////////////////////////////////////////////////////////////////////
//other
NTSTATUS NTAPI Hook_NtQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
											 IN OUT PVOID SystemInformation,
											 IN ULONG SystemInformationLength,
											 OUT PULONG ReturnLength OPTIONAL
											 );

//////////////////////////////////////////////////////////////////////////
#define CHANGE_OBJ_NAME(codeToCall)	\
	if (ObjectAttributes \
	&& ObjectAttributes->ObjectName \
	&& ObjectAttributes->ObjectName->Buffer \
	&& ObjectAttributes->ObjectName->Length )\
{\
	NTSTATUS ret = -1;\
	PUNICODE_STRING pOldName =  ObjectAttributes->ObjectName;\
	UNICODE_STRING struW = {0};\
	std::wstring strNewName(pOldName->Buffer,pOldName->Length/sizeof(wchar_t));\
	strNewName += g_pData->GetNewNameW();\
	struW.Buffer = (wchar_t*)strNewName.c_str();\
	struW.Length = struW.MaximumLength = strNewName.length()*sizeof(wchar_t);\
	ObjectAttributes->ObjectName = &struW;\
	ret = codeToCall;\
	ObjectAttributes->ObjectName = pOldName;\
	return ret;\
}\
	return codeToCall

#define TRY_TO_CHANGE_OBJ_NAME(codeToCall)	\
	NTSTATUS ret = -1;\
	if (ObjectAttributes \
	&& ObjectAttributes->ObjectName	\
	&& ObjectAttributes->ObjectName->Buffer\
	&& ObjectAttributes->ObjectName->Length)\
{\
	PUNICODE_STRING pOldName =  ObjectAttributes->ObjectName;\
	UNICODE_STRING struW = {0};\
	std::wstring strNewName(pOldName->Buffer,pOldName->Length/sizeof(wchar_t));\
	strNewName += g_pData->GetNewNameW();	\
	struW.Buffer = (wchar_t*)strNewName.c_str();\
	struW.Length = struW.MaximumLength = strNewName.length()*sizeof(wchar_t);\
	ObjectAttributes->ObjectName = &struW;	\
	ret = codeToCall;\
	ObjectAttributes->ObjectName = pOldName;\
}\
	if( ret < 0 )\
{\
	ret = codeToCall;\
}\
	return ret
