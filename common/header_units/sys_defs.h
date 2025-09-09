// ReSharper disable CppUnusedIncludeDirective
#pragma once
#ifndef _SYS_DEFS_H_
#define _SYS_DEFS_H_

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define NOMINMAX
// Windows 头文件
#include <windows.h>
#include <windowsx.h>
#include <winternl.h>
#include <WinIoCtl.h>
#include <VersionHelpers.h>
#include <tlhelp32.h>
#include <winhttp.h>
#include <dbghelp.h>
#include <detours.h>
#if defined(_MSC_VER) && defined(__INTELLISENSE__)
#pragma message("Just for IntelliSense. You should not see this message!")
// msvc的INTELLISENSE在模块中无法解析__declspec(nothrow)
// 定义这个宏专门伺候__INTELLISENSE__
#define COM_STDMETHOD_CAN_THROW
#endif
#include <d2d1.h>
#include <dwrite.h>

#include <shellapi.h>
#include <Nb30.h>
#include <winsock2.h>
#include <Iphlpapi.h>

enum class ArchBit
{
	Bit32,
	Bit64
};

inline constexpr bool IS_CURRENT_ARCH_32_BIT = sizeof(void*) == 4;
inline constexpr bool IS_CURRENT_ARCH_64_BIT = sizeof(void*) == 8;
static_assert(IS_CURRENT_ARCH_64_BIT == !IS_CURRENT_ARCH_32_BIT);

inline constexpr ArchBit CURRENT_ARCH_BIT = IS_CURRENT_ARCH_32_BIT ? ArchBit::Bit32 : ArchBit::Bit64;

template <class Interface>
class UniqueComPtr
{
public:
	UniqueComPtr() noexcept = default;
	UniqueComPtr(const UniqueComPtr&) = delete;
	UniqueComPtr& operator=(const UniqueComPtr&) = delete;

	UniqueComPtr(UniqueComPtr&& that) noexcept : m_p(that.m_p)
	{
		that.m_p = nullptr;
	}

	~UniqueComPtr()
	{
		if (m_p)
		{
			m_p->Release();
		}
	}

	void reset(Interface* p = nullptr) noexcept
	{
		if (m_p)
		{
			m_p->Release();
		}
		m_p = p;
	}

	void swap(UniqueComPtr& that) noexcept
	{
		Interface* tmp = m_p;
		m_p = that.m_p;
		that.m_p = tmp;
	}

	friend void swap(UniqueComPtr& p1, UniqueComPtr& p2) noexcept
	{
		p1.swap(p2);
	}

	UniqueComPtr& operator=(UniqueComPtr&& that) noexcept
	{
		UniqueComPtr(static_cast<UniqueComPtr&&>(that)).swap(*this);
		return *this;
	}

	Interface** operator&() noexcept
	{
		return &m_p;
	}

	Interface* operator->() const noexcept
	{
		return m_p;
	}

	operator Interface*() const noexcept
	{
		return m_p;
	}

	Interface* get() const noexcept
	{
		return m_p;
	}

	explicit operator bool() const noexcept
	{
		return m_p != nullptr;
	}

	bool operator!() const noexcept
	{
		return m_p == nullptr;
	}

private:
	Interface* m_p{nullptr};
};

struct SystemVersionInfo
{
	bool isWindows8Point1OrGreater;
	bool isWindows8OrGreater;
	bool isWindowsVistaOrGreater;
	bool is32BitSystem;
};

struct NtdllSymbolRvaInfo
{
	ULONGLONG LdrpHandleTlsData;
	ULONGLONG LdrpInvertedFunctionTable;
	ULONGLONG RtlInsertInvertedFunctionTable;
	ULONGLONG LdrpReleaseTlsEntry;
};

struct Kernel32DllInfo
{
	DWORD rvaLoadLibraryA;
	DWORD rvaGetProcAddress;
	DWORD rvaFlushInstructionCache;
};

struct EssentialData
{
	SystemVersionInfo version;

	NtdllSymbolRvaInfo symRva32;
	NtdllSymbolRvaInfo symRva64;

	Kernel32DllInfo kernelInfo32;
	Kernel32DllInfo kernelInfo64;
};

struct DllInjectionInfo
{
	ULONGLONG kernelDllAddress;
	ULONGLONG dllAddress;
	DWORD dllSize;
	DWORD rvaRelocation;
	DWORD rvaImportDir;
	DWORD rvaEntryPoint;
	ULONGLONG desiredImageBase;
};

#pragma warning(push)
#pragma warning(disable: 4200)
struct ReflectiveInjectParams
{
	EssentialData essentialData;
	DllInjectionInfo injectionInfo;
	ULONGLONG envFlag;
	DWORD envIndex;
	DWORD rootPathCount;
	wchar_t rootPath[];
};
#pragma warning(pop)

// {C01FA6E2-92EB-4418-8D1A-F6307540CC19}
inline constexpr GUID DETOUR_INJECT_PARAMS_GUID = {0xc01fa6e2, 0x92eb, 0x4418, {0x8d, 0x1a, 0xf6, 0x30, 0x75, 0x40, 0xcc, 0x19}};
#pragma warning(push)
#pragma warning(disable: 4200)
struct DetourInjectParams
{
	SystemVersionInfo version;
	ULONGLONG envFlag;
	DWORD envIndex;
	DWORD rootPathCount;
	wchar_t rootPath[];
};
#pragma warning(pop)
#endif
