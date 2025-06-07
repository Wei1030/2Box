#pragma once
#ifndef _SYS_DEFS_H_
#define _SYS_DEFS_H_

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
#define NOMINMAX
// Windows 头文件
#include <windows.h>
#include <tlhelp32.h>
#include <d2d1.h>

template <class Interface>
inline void safe_release(Interface** ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != nullptr)
	{
		(*ppInterfaceToRelease)->Release();
		(*ppInterfaceToRelease) = nullptr;
	}
}





#endif
