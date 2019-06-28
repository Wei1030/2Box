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

	template<typename FuncPtrType>
	BOOL InitTrampolineFunc(
		CTrampolineFunc<FuncPtrType>* pTrampoline,
		HMODULE hMod,
		const char* pszProcName,
		FuncPtrType pfnDetour ,		 
		BOOL bFromFile = false);

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
	BOOL bFromFile /*= false*/)
{
	FuncPtrType pfnSrc = NULL;
	DWORD pRva = 0;

	if (bFromFile
		&& m_bFileCreateSucc
		&& (pRva = m_fm.GetProcRVA(pszProcName)))
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

#define TRAMPOLINE(functype,procname) \
	CTrampolineFunc<functype> True##procname;

#define HOOK(classname,hmod,procname) \
	CBaseHook::InitTrampolineFunc(&True##procname,hmod,#procname,&classname::##procname);

//�������ģʽ����ʱ�����ֺ����ᱻ΢��������ֿ⣨Acllayer.dll �ȿ⣩IAT hook����ʹ�ô˺�hook��ȷ��API��ַ;
#define HOOK_FROM_FILE_EAT(classname,hmod,procname) \
	CBaseHook::InitTrampolineFunc(&True##procname,hmod,#procname,&classname::##procname,TRUE);

