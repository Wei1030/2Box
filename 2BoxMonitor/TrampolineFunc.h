#pragma once

class CTrampolineFuncBase
{
public:
	CTrampolineFuncBase();	
	CTrampolineFuncBase(PROC sourceFunc,PROC detourFunc);	
	//~CTrampolineFuncBase();

	static CTrampolineFuncBase* GetHead() {return s_pHead;}
	static BOOL HookAll();
public:	
	BOOL RestoreThisHook();

protected:
	PROC m_sourceFunc;
	PROC m_detourFunc;

private:
	static CTrampolineFuncBase* s_pHead;
	CTrampolineFuncBase* m_pNext;
};

template<typename FuncPtrType>
class CTrampolineFunc : public CTrampolineFuncBase
{
public:
	CTrampolineFunc(): CTrampolineFuncBase()
	{
	}

	CTrampolineFunc(FuncPtrType sourceFunc,FuncPtrType detourFunc)
		: CTrampolineFuncBase((PROC)sourceFunc,(PROC)detourFunc)
	{
	}

	inline void SetHook(FuncPtrType sourceFunc,FuncPtrType detourFunc)
	{
		m_sourceFunc = (PROC)sourceFunc;
		m_detourFunc = (PROC)detourFunc;
	}

	inline FuncPtrType Call() {return (FuncPtrType)m_sourceFunc;}
};
