// ReSharper disable CppUnusedIncludeDirective
#pragma once
#ifndef _SYS_DEFS_H_
#define _SYS_DEFS_H_

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define NOMINMAX
// Windows 头文件
#include <windows.h>
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
inline void safe_release(Interface** ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != nullptr)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = nullptr;
	}
}

struct ReflectiveInjectParams
{
	ULONGLONG kernel32Address;
	DWORD loadLibraryARVA;
	DWORD getProcAddressRVA;
	DWORD flushInstructionCacheRVA;
	
	ULONGLONG dllFileAddress;
	DWORD dllFileSize;
	DWORD dllRelocationRVA;
	DWORD dllImportDirRVA;
	ULONGLONG dllImageBase;
	DWORD entryPointRVA;
};



#endif
