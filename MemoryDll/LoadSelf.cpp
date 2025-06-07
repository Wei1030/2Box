#ifndef _WIN64
#pragma comment(linker, "/EXPORT:load_self=_load_self@4")
#endif

import Utility.InjectLib;

// 这个函数用来加载自身，全程必须位置无关。做到不调用任何外部函数和数据
extern "C" __declspec(dllexport) unsigned long __stdcall load_self(void* lpThreadParameter)
{
	utils::ReflectiveInjectParams* injectParams = static_cast<utils::ReflectiveInjectParams*>(lpThreadParameter);
	if (!injectParams)
	{
		return 0;
	}
	
	return 0;
}
