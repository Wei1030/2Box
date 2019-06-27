#pragma once
#include "BaseHook.h"
#include <ShellAPI.h>

class CShell32Hook :public CBaseHook
{
public:
	CShell32Hook(void);
	~CShell32Hook(void);

	virtual BOOL Init(CDbghelpWrapper* pHelper) override;

private:
	static BOOL STDAPICALLTYPE ShellExecuteExA(__inout LPSHELLEXECUTEINFOA lpExecInfo);
	static BOOL STDAPICALLTYPE ShellExecuteExW(__inout LPSHELLEXECUTEINFOW lpExecInfo);
};
