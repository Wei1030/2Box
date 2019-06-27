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
	static BOOL HookAll(CDbghelpWrapper* pHelper);	

	virtual BOOL Init(CDbghelpWrapper* pHelper){ return FALSE;}

protected:
	BOOL InitFakeFile(const wchar_t* pszSysFileName);
	void SavedAndUnmapFakeFile();

	template<typename FuncPtrType>
	BOOL InitTrampolineFunc(
		CTrampolineFunc<FuncPtrType>* pTrampoline,
		HMODULE hMod,
		const char* pszProcName,
		FuncPtrType pfnDetour ,
		CDbghelpWrapper* pHelper,
		BOOL bFromFile = false);

	template<typename FuncPtrType>
	BOOL InitTrampolineFuncInFakeFile(
		CTrampolineFunc<FuncPtrType>* pTrampoline,
		const char* pszProcName,
		FuncPtrType pfnDetour ,
		CDbghelpWrapper* pHelper);

private:
	static CBaseHook* s_pHead;
	CBaseHook* m_pNext;
	CFileModule m_fm;
	BOOL m_bFileCreateSucc;
};

template<typename FuncPtrType>
BOOL CBaseHook::InitTrampolineFunc(
	CTrampolineFunc<FuncPtrType>* pTrampoline, 
	HMODULE hMod, 
	const char* pszProcName, 
	FuncPtrType pfnDetour , 
	CDbghelpWrapper* pHelper, 
	BOOL bFromFile /*= false*/)
{
	FuncPtrType pfnSrc = NULL;
	DWORD pRva = 0;

	if (bFromFile
		&& m_bFileCreateSucc
		&& pHelper
		&& (pRva = m_fm.GetProcRVA(pszProcName,pHelper)))
	{
		pfnSrc = (FuncPtrType)((size_t)hMod + pRva);
	}
	else
	{
		pfnSrc = (FuncPtrType)GetProcAddress(hMod,pszProcName);
	}

	pTrampoline->SetHook(pfnSrc,pfnDetour);

	return TRUE;
}

template<typename FuncPtrType>
BOOL CBaseHook::InitTrampolineFuncInFakeFile(
	CTrampolineFunc<FuncPtrType>* pTrampoline, 
	const char* pszProcName, 
	FuncPtrType pfnDetour , 
	CDbghelpWrapper* pHelper)
{

	if (FALSE == m_bFileCreateSucc
		|| NULL == pHelper)
	{
		return FALSE;
	}

	FuncPtrType pSrcInFile = (FuncPtrType)m_fm.GetProcVA(pszProcName,pHelper);
	pTrampoline->SetHook(pSrcInFile,pfnDetour);
	return TRUE;	
}

#define TRAMPOLINE(functype,procname) \
	CTrampolineFunc<functype> True##procname;\
	CTrampolineFunc<functype> True##procname##Infile

#define HOOK(classname,hmod,procname,helper) \
	CBaseHook::InitTrampolineFunc(&True##procname,hmod,#procname,&classname::##procname,helper);\
	CBaseHook::InitTrampolineFuncInFakeFile(&True##procname##Infile,#procname,&classname::##procname,helper)

//程序兼容模式运行时，部分函数会被微软兼容助手库（Acllayer.dll 等库）IAT hook，可使用此宏hook正确的API地址;
#define HOOK_FROM_FILE_EAT(classname,hmod,procname,helper) \
	CBaseHook::InitTrampolineFunc(&True##procname,hmod,#procname,&classname::##procname,helper,TRUE);\
	CBaseHook::InitTrampolineFuncInFakeFile(&True##procname##Infile,#procname,&classname::##procname,helper)

