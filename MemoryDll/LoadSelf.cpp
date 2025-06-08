#ifndef _WIN64
#pragma comment(linker, "/EXPORT:load_self=_load_self@4")
#endif

import Utility.InjectLib;
import Utility.PEParser;

namespace
{
	constexpr unsigned int HASH_KEY = 13;
	constexpr unsigned int HASH_LOAD_LIBRARY_A = 3960360590;
	constexpr unsigned int HASH_GET_PROC_ADDRESS = 2081291434;
	constexpr unsigned int HASH_FLUSH_INSTRUCTION_CACHE = 1393691008;

	unsigned int my_hash(const char* c)
	{
		unsigned int h = 0;
		do
		{
			h = (h >> HASH_KEY) | (h << (32 - HASH_KEY));
			h += *c;
		} while (*++c);

		return h;
	}
}

template <unsigned int ApiHash>
struct CmpHelper
{
	bool operator()(const char* funcName) const
	{
		if (funcName == nullptr)
		{
			return false;
		}
		return my_hash(funcName)  == ApiHash;
	}
};

// 这个函数用来加载自身，被调用时，虽然已经进行过内存对齐，但是重定位修复和导入函数都未做处理
// 因此不得使用位置相关的静态数据，不能调用导入函数。不得使用异常或执行流跳转。尽量不调用外部函数
extern "C" __declspec(dllexport) unsigned long __stdcall load_self(void* lpThreadParameter)
{
	if (!lpThreadParameter)
	{
		return 0;
	}

	utils::ReflectiveInjectParams* injectParams = static_cast<utils::ReflectiveInjectParams*>(lpThreadParameter);
	utils::PEParser<utils::PEParserFlag::hasMapped | utils::PEParserFlag::noThrow> kernel32Parser(reinterpret_cast<const char*>(injectParams->kernel32BaseAddress), injectParams->kernel32Size);
	unsigned long offset = kernel32Parser.getProcOffset(CmpHelper<HASH_LOAD_LIBRARY_A>{});

	
	return  0;
}
