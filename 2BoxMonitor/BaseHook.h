#pragma once

#include "FileModule.h"

template<typename FuncPtrType>
class CTrampolineFunc;

class CBaseHook
{
public:
	CBaseHook(void);
	virtual ~CBaseHook(void);

	static CBaseHook* GetHead() {return s_pHead;}
	static BOOL HookAll();	

	virtual BOOL Init(){ return FALSE;}

protected:
	BOOL InitFile(const wchar_t* pszSysFileName,BOOL bDenyAcess = TRUE);
	void UninitFile();

	enum WHERE_IS_SRC
	{
		STATIC_IN_MEMORY,
		LOAD_FROM_MEMORY,

		//程序兼容模式运行时，部分函数会被微软兼容助手库（Acllayer.dll 等库）IAT hook;
		//可使用LOAD_FROM_FILE 获得正确的API地址;
		LOAD_FROM_FILE
	};

	template<typename FuncPtrType>
	BOOL InitTrampolineFunc(
		CTrampolineFunc<FuncPtrType>* pTrampoline,
		HMODULE hMod,
		const void* srcFnAddrOrSrcName,
		FuncPtrType pfnDetour ,		 
		WHERE_IS_SRC whereSrc = STATIC_IN_MEMORY);

private:
	static CBaseHook* s_pHead;
	CBaseHook* m_pNext;
	CFileModule m_fm;
	BOOL m_bFileCreateSucc;
};

template<typename FuncPtrType>
BOOL CBaseHook::InitTrampolineFunc( CTrampolineFunc<FuncPtrType>* pTrampoline, 
								   HMODULE hMod, 
								   const void* srcFnAddrOrSrcName, 
								   FuncPtrType pfnDetour , 
								   WHERE_IS_SRC whereSrc /*= STATIC_IN_MEMORY*/ )
{
	FuncPtrType pfnSrc = NULL;
	DWORD pRva = 0;

	if (whereSrc == STATIC_IN_MEMORY)
	{
		pfnSrc = (FuncPtrType)srcFnAddrOrSrcName;
	}
	else if (hMod)
	{
		const char* pszSrcName = (const char*)srcFnAddrOrSrcName;

		if (whereSrc == LOAD_FROM_MEMORY)
		{
			pfnSrc = (FuncPtrType)GetProcAddress(hMod,pszSrcName);
		}
		else if (whereSrc == LOAD_FROM_FILE
			&& m_bFileCreateSucc
			&& (pRva = m_fm.GetProcRVA(pszSrcName)))
		{
			pfnSrc = (FuncPtrType)((size_t)hMod + pRva);
		}
	}
	
	pTrampoline->SetHook(pfnSrc,pfnDetour);
	return TRUE;
}

#define TRAMPOLINE(functype,procname) \
	CTrampolineFunc<functype> True##procname;

#define HOOK(classname,procname) \
	CBaseHook::InitTrampolineFunc(&True##procname,NULL,&::procname,&classname::##procname,STATIC_IN_MEMORY);

#define HOOK_SRC_FROM_MEM(classname,hmod,procname) \
	CBaseHook::InitTrampolineFunc(&True##procname,hmod,#procname,&classname::##procname,LOAD_FROM_MEMORY);

//程序兼容模式运行时，部分函数会被微软兼容助手库（Acllayer.dll 等库）IAT hook，可使用此宏hook正确的API地址;
#define HOOK_SRC_FROM_FILE_EAT(classname,hmod,procname) \
	CBaseHook::InitTrampolineFunc(&True##procname,hmod,#procname,&classname::##procname,LOAD_FROM_FILE);

