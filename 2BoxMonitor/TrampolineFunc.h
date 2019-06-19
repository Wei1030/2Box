#pragma once

class CTrampolineFuncBase
{
public:
	CTrampolineFuncBase(PROC sourceFunc,PROC detourFunc);
	CTrampolineFuncBase(LPCWSTR lpLibFileName,LPCSTR lpProcName,PROC detourFunc);
	CTrampolineFuncBase(LPCSTR lpNtProcName,PROC detourFunc);
	//~CTrampolineFuncBase();

	static CTrampolineFuncBase* GetHead() {return s_pHead;}

public:
	BOOL HookAll();
	BOOL RestoreThisHook();

protected:
	PROC m_sourceFunc;
	PROC m_detourFunc;

private:
	static CTrampolineFuncBase* s_pHead;
	static HMODULE s_hNtdll;
	CTrampolineFuncBase* m_pNext;
};

template<typename FuncPtrType>
class CTrampolineFunc : public CTrampolineFuncBase
{
public:
	CTrampolineFunc(FuncPtrType sourceFunc,FuncPtrType detourFunc)
		: CTrampolineFuncBase((PROC)sourceFunc,(PROC)detourFunc)
	{
	}

	CTrampolineFunc(LPCWSTR lpLibFileName,LPCSTR lpProcName,FuncPtrType detourFunc)
		: CTrampolineFuncBase(lpLibFileName,lpProcName,(PROC)detourFunc)
	{
	}

	CTrampolineFunc(LPCSTR lpNtProcName,FuncPtrType detourFunc)
		: CTrampolineFuncBase(lpNtProcName,(PROC)detourFunc)
	{
	}

	inline FuncPtrType Call() {return (FuncPtrType)m_sourceFunc;}
};
