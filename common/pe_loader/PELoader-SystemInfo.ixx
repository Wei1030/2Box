export module PELoader:SystemInfo;

import std;
import "sys_defs.h";

namespace pe
{
	// win8后系统版本的准确获取需要应用程序有对应的manifest，因此系统版本由主进程填充，注入到其他进程的内存dll直接拿
	export SystemVersionInfo g_os_version;

	// win8前, 内存dll在初始化静态TLS前无法使用静态局部变量，只能拆出一个接口由主进程初始化一次
	export void fill_os_version()
	{
		g_os_version.isWindows8Point1OrGreater = IsWindows8Point1OrGreater();
		g_os_version.isWindows8OrGreater = IsWindows8OrGreater();
		g_os_version.isWindowsVistaOrGreater = IsWindowsVistaOrGreater();
	}

	export void fill_os_version(const SystemVersionInfo& version)
	{
		g_os_version = version;
	}
}
