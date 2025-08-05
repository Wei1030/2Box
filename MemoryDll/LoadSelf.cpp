// ReSharper disable CppClangTidyPerformanceNoIntToPtr
// ReSharper disable CppClangTidyCppcoreguidelinesAvoidGoto
#ifndef _WIN64
#pragma comment(linker, "/EXPORT:load_self=_load_self@4")
#endif

#include "sys_defs.h"


// 这个函数用来加载自身，被调用时，虽然已经进行过内存对齐，但是重定位修复和导入函数都未做处理
// 因此不得使用位置相关的静态数据，不能调用导入函数。不得使用异常或执行流跳转。尽量不调用外部函数
extern "C" __declspec(dllexport) unsigned long __stdcall load_self(void* lpThreadParameter)
{
	if (!lpThreadParameter)
	{
		return 0;
	}

	int result = 0;
	const ReflectiveInjectParams& injectParams = *static_cast<ReflectiveInjectParams*>(lpThreadParameter);
	const EssentialData& essentialData = injectParams.essentialData;
	const DllInjectionInfo& injectionCtx = injectParams.injectionInfo;
#ifdef _WIN64
	const Kernel32DllInfo& kernel32DllInfo = essentialData.kernelInfo64;
#else
	const Kernel32DllInfo& kernel32DllInfo = essentialData.kernelInfo32;
#endif
	
	auto pLoadLibraryA = reinterpret_cast<decltype(&LoadLibraryA)>(injectionCtx.kernelDllAddress + kernel32DllInfo.rvaLoadLibraryA);
	auto pGetProcAddress = reinterpret_cast<decltype(&GetProcAddress)>(injectionCtx.kernelDllAddress + kernel32DllInfo.rvaGetProcAddress);
	auto pFlushInstructionCache = reinterpret_cast<decltype(&FlushInstructionCache)>(injectionCtx.kernelDllAddress + kernel32DllInfo.rvaFlushInstructionCache);
	char* pNewBase = reinterpret_cast<char*>(injectionCtx.dllAddress);
	typedef BOOL (__stdcall*PtrDllMain)(HINSTANCE, DWORD, LPVOID);
	PtrDllMain pfnDllMain = reinterpret_cast<PtrDllMain>(pNewBase + injectionCtx.rvaEntryPoint);

	// 处理重定位表
	if (size_t baseDelta = static_cast<size_t>(injectionCtx.dllAddress - injectionCtx.desiredImageBase))
	{
		IMAGE_BASE_RELOCATION* pLoc = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pNewBase + injectionCtx.rvaRelocation);
		while (pLoc->VirtualAddress)
		{
			char* pCurrentLoc = reinterpret_cast<char*>(pLoc);
			char* pCurrentItem = pCurrentLoc + sizeof(IMAGE_BASE_RELOCATION);
			const DWORD numberOfRelocation = (pLoc->SizeOfBlock - sizeof(IMAGE_BASE_RELOCATION)) / sizeof(WORD);
			for (DWORD i = 0; i < numberOfRelocation; ++i)
			{
				const WORD item = *reinterpret_cast<WORD*>(pCurrentItem + i * sizeof(WORD));
				const WORD type = (item & 0xF000) >> 12;
				const WORD offset = item & 0x0FFF;
				if (type == IMAGE_REL_BASED_DIR64)
				{
					ULONG_PTR* pNeedRelocation = reinterpret_cast<ULONG_PTR*>(pNewBase + pLoc->VirtualAddress + offset);
					*pNeedRelocation += baseDelta;
				}
				else if (type == IMAGE_REL_BASED_HIGHLOW)
				{
					DWORD* pNeedRelocation = reinterpret_cast<DWORD*>(pNewBase + pLoc->VirtualAddress + offset);
					*pNeedRelocation += static_cast<DWORD>(baseDelta);
				}
				else if (type == IMAGE_REL_BASED_HIGH)
				{
					WORD* pNeedRelocation = reinterpret_cast<WORD*>(pNewBase + pLoc->VirtualAddress + offset);
					*pNeedRelocation += HIWORD(baseDelta);
				}
				else if (type == IMAGE_REL_BASED_LOW)
				{
					WORD* pNeedRelocation = reinterpret_cast<WORD*>(pNewBase + pLoc->VirtualAddress + offset);
					*pNeedRelocation += LOWORD(baseDelta);
				}
			}
			pLoc = reinterpret_cast<IMAGE_BASE_RELOCATION*>(pCurrentLoc + pLoc->SizeOfBlock);
		}
	}

	// 处理导入表
	if (injectionCtx.rvaImportDir)
	{
		using ThunkType = decltype(IMAGE_ORDINAL_FLAG);

		IMAGE_IMPORT_DESCRIPTOR* pImportDir = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(pNewBase + injectionCtx.rvaImportDir);
		while (pImportDir->FirstThunk)
		{
			const char* dllName = pNewBase + pImportDir->Name;
			HMODULE hDll = pLoadLibraryA(dllName);
			if (!hDll)
			{
				goto fail;
			}

			ThunkType* pINT = reinterpret_cast<ThunkType*>(pNewBase + pImportDir->OriginalFirstThunk);
			ThunkType* pIAT = reinterpret_cast<ThunkType*>(pNewBase + pImportDir->FirstThunk);
			ThunkType thunkData = *pINT;
			while (thunkData)
			{
				FARPROC lpFunction;
				if (thunkData & IMAGE_ORDINAL_FLAG)
				{
					// 存的是序号
					lpFunction = pGetProcAddress(hDll, reinterpret_cast<LPCSTR>(IMAGE_ORDINAL(thunkData)));
				}
				else
				{
					// 存的是名字
					IMAGE_IMPORT_BY_NAME* nameTable = reinterpret_cast<IMAGE_IMPORT_BY_NAME*>(pNewBase + thunkData);
					lpFunction = pGetProcAddress(hDll, nameTable->Name);
				}
				if (lpFunction)
				{
					*pIAT = reinterpret_cast<ThunkType>(lpFunction);
				}
				else
				{
					goto fail;
				}
				pINT++;
				pIAT++;
				// pINT = reinterpret_cast<ThunkType*>(reinterpret_cast<char*>(pINT) + sizeof(ThunkType));
				// pIAT = reinterpret_cast<ThunkType*>(reinterpret_cast<char*>(pIAT) + sizeof(ThunkType));
				thunkData = *pINT;
			}
			// pImportDir++;
			pImportDir = reinterpret_cast<IMAGE_IMPORT_DESCRIPTOR*>(reinterpret_cast<char*>(pImportDir) + sizeof(IMAGE_IMPORT_DESCRIPTOR));
		}
	}

	pFlushInstructionCache(reinterpret_cast<HANDLE>(-1), pNewBase, injectionCtx.dllSize);

	if (!pfnDllMain(reinterpret_cast<HINSTANCE>(pNewBase), DLL_PROCESS_ATTACH, nullptr))
	{
		goto fail;
	}
	result = 1;

fail:
	// 出错时直接让其除零崩溃
	return 1 / result;
}
