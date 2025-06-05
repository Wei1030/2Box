#ifndef _WIN64
#pragma comment(linker, "/EXPORT:load_self=_load_self@4")
#endif

// 这个函数用来加载自身，全程必须位置无关。做到不调用任何外部函数和数据
extern "C" __declspec(dllexport) unsigned long __stdcall load_self(void* lpThreadParameter)
{
	if (!lpThreadParameter)
	{
		return 0;
	}
	return 0;
}
