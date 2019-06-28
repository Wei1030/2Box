#pragma once
#include "BaseHook.h"
#include <Winternl.h>

class CNtdllHook : public CBaseHook
{
public:
	CNtdllHook(void);
	~CNtdllHook(void);

	virtual BOOL Init() override;

private:
	//////////////////////////////////////////////////////////////////////////
	//Event
	static NTSTATUS NTAPI  NtCreateEvent(OUT PHANDLE EventHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN int EventType,
		IN BOOLEAN InitialState);

	static NTSTATUS NTAPI NtOpenEvent(OUT PHANDLE EventHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes);

	//////////////////////////////////////////////////////////////////////////
	//Mutant
	static NTSTATUS NTAPI  NtCreateMutant(OUT PHANDLE MutantHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN BOOLEAN InitialOwner);

	static NTSTATUS NTAPI NtOpenMutant(OUT PHANDLE MutantHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes);

	//////////////////////////////////////////////////////////////////////////
	//Section
	static NTSTATUS NTAPI NtCreateSection(OUT PHANDLE SectionHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN PLARGE_INTEGER SectionSize OPTIONAL,
		IN ULONG Protect,
		IN ULONG Attributes,
		IN HANDLE FileHandle
		);

	static NTSTATUS NTAPI NtOpenSection(OUT PHANDLE SectionHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes
		);

	//////////////////////////////////////////////////////////////////////////
	//Semaphore
	static NTSTATUS NTAPI NtCreateSemaphore( OUT PHANDLE SemaphoreHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN LONG InitialCount,
		IN LONG MaximumCount);

	static NTSTATUS NTAPI NtOpenSemaphore(OUT PHANDLE SemaphoreHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes
		);

	//////////////////////////////////////////////////////////////////////////
	//Timer
	static NTSTATUS NTAPI  NtCreateTimer(OUT PHANDLE TimerHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		IN int TimerType);

	static NTSTATUS NTAPI NtOpenTimer(OUT PHANDLE TimerHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes);

	//////////////////////////////////////////////////////////////////////////
	//Job
	static NTSTATUS NTAPI NtCreateJobObject(OUT PHANDLE JobHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes);

	static NTSTATUS NTAPI NtOpenJobObject(OUT PHANDLE JobHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes);

	//////////////////////////////////////////////////////////////////////////
	//NamedPipe
	static NTSTATUS NTAPI  NtCreateNamedPipeFile(	_Out_ PHANDLE 	FileHandle,
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

	static NTSTATUS NTAPI  NtCreateFile(OUT PHANDLE FileHandle,
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

	static NTSTATUS NTAPI NtOpenFile(OUT PHANDLE FileHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN ULONG ShareAccess,
		IN ULONG OpenOptions);

	//////////////////////////////////////////////////////////////////////////
	//reg key
	// static NTSTATUS NTAPI  NtCreateKey(OUT PHANDLE KeyHandle,
	// 								 IN ACCESS_MASK DesiredAccess,
	// 								 IN POBJECT_ATTRIBUTES ObjectAttributes,
	// 								 IN ULONG TitleIndex,
	// 								 IN PUNICODE_STRING Class OPTIONAL,
	// 								 IN ULONG CreateOptions,
	// 								 OUT PULONG Disposition OPTIONAL);

	// static NTSTATUS NTAPI NtOpenKey(OUT PHANDLE KeyHandle,
	// 							  IN ACCESS_MASK DesiredAccess,
	// 							  IN POBJECT_ATTRIBUTES ObjectAttributes);

	// static NTSTATUS NTAPI NtRenameKey(IN HANDLE KeyHandle,IN PUNICODE_STRING ReplacementName);

	// static NTSTATUS NTAPI NtSetValueKey(IN HANDLE KeyHandle,
	// 								  IN PUNICODE_STRING ValueName,
	// 								  IN ULONG TitleIndex,
	// 								  IN ULONG Type,
	// 								  IN PVOID Data,
	// 								  IN ULONG DataSize );

	// static NTSTATUS NTAPI NtAlpcConnectPort(	_Out_ PHANDLE 	PortHandle,
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
	static NTSTATUS NTAPI NtQuerySystemInformation(IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
		IN OUT PVOID SystemInformation,
		IN ULONG SystemInformationLength,
		OUT PULONG ReturnLength OPTIONAL
		);

	//////////////////////////////////////////////////////////////////////////
};


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
	struW.Length = struW.MaximumLength = (USHORT)strNewName.length()*sizeof(wchar_t);\
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
	struW.Length = struW.MaximumLength = (USHORT)strNewName.length()*sizeof(wchar_t);\
	ObjectAttributes->ObjectName = &struW;	\
	ret = codeToCall;\
	ObjectAttributes->ObjectName = pOldName;\
}\
	if( ret < 0 )\
{\
	ret = codeToCall;\
}\
	return ret
