#pragma once

class CDbghelpWrapper;
class CKernel32Hook;
class COle32Hook;
class CIphlpapiHook;
class CNetapi32Hook;
class CAdvapi32Hook;
class CShell32Hook;
class CUser32Hook;
class CNtdllHook;

class CHookMgr
{
public:
	CHookMgr(void);
	~CHookMgr(void);

	BOOL Init(CDbghelpWrapper* pHelper);

private:
	CKernel32Hook* m_pKernel32Hook;
	COle32Hook* m_pOle32Hook;
	CIphlpapiHook* m_pIphlpapiHook;
	CNetapi32Hook* m_pNetapi32Hook;
	CAdvapi32Hook* m_pAdvapi32Hook;
	CShell32Hook* m_pShell32Hook;
	CUser32Hook* m_pUser32Hook;
	CNtdllHook* m_pNtdllHook;
};
