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
#include <VersionHelpers.h>
#include <tlhelp32.h>
#include <winhttp.h>
#include <dbghelp.h>
#if defined(_MSC_VER) && defined(__INTELLISENSE__)
#pragma message("Just for IntelliSense. You should not see this message!")
// msvc的INTELLISENSE在模块中无法解析__declspec(nothrow)
// 定义这个宏专门伺候__INTELLISENSE__
#define COM_STDMETHOD_CAN_THROW
#endif
#include <d2d1.h>
#include <dwrite.h>

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

struct ReflectiveInjectParams
{
	ULONGLONG kernel32Address;
	DWORD loadLibraryARVA;
	DWORD getProcAddressRVA;
	DWORD flushInstructionCacheRVA;

	ULONGLONG dllMemAddress;
	DWORD dllMemSize;
	DWORD dllRelocationRVA;
	DWORD dllImportDirRVA;
	ULONGLONG dllImageBase;
	DWORD entryPointRVA;
	
	bool isWindows8Point1OrGreater;
	bool isWindows8OrGreater;
	
	ULONGLONG rvaLdrpHandleTlsData32 = 0;
	ULONGLONG rvaLdrpHandleTlsData64 = 0;
	ULONGLONG rvaLdrpInvertedFunctionTable32 = 0;
	ULONGLONG rvaLdrpInvertedFunctionTable64 = 0;
	ULONGLONG rvaRtlInsertInvertedFunctionTable32 = 0;
	ULONGLONG rvaRtlInsertInvertedFunctionTable64 = 0;
	ULONGLONG rvaLdrpReleaseTlsEntry32 = 0;
	ULONGLONG rvaLdrpReleaseTlsEntry64 = 0;
};


#endif
