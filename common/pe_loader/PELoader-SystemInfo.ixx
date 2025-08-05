export module PELoader:SystemInfo;

import std;
import "sys_defs.h";

namespace pe
{
	// win8后系统版本的准确获取需要应用程序有对应的manifest，因此系统版本由主进程填充，注入到其他进程的内存dll直接拿
	export SystemVersionInfo g_os_version;

	export void fill_os_version(const SystemVersionInfo& version)
	{
		g_os_version = version;
	}
}
