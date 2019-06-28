#pragma once
#include "BaseHook.h"
#include <Nb30.h>

class CNetapi32Hook : public CBaseHook
{
public:
	CNetapi32Hook(void);
	~CNetapi32Hook(void);

	virtual BOOL Init() override;

private:
	static UCHAR APIENTRY Netbios(PNCB pcnb);
};
