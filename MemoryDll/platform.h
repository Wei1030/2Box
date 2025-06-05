#pragma once
#include <winsdkver.h>

#define WINVER			0x0601
#define _WIN32_WINNT	0x0601

#include <SDKDDKVer.h>

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
