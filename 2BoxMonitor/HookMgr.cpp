#include "StdAfx.h"
#include "HookMgr.h"
#include "Kernel32Hook.h"
#include "Ole32Hook.h"
#include "IphlpapiHook.h"
#include "Netapi32Hook.h"
#include "Advapi32Hook.h"
#include "User32Hook.h"
#include "NtdllHook.h"

CHookMgr::CHookMgr(void)
{
	m_pIphlpapiHook	= new CIphlpapiHook;
	m_pOle32Hook	= new COle32Hook;
	m_pAdvapi32Hook = new CAdvapi32Hook;
	m_pNetapi32Hook = new CNetapi32Hook;
	m_pUser32Hook	= new CUser32Hook;
	m_pKernel32Hook = new CKernel32Hook;
	m_pNtdllHook	= new CNtdllHook;
}

CHookMgr::~CHookMgr(void)
{
}

BOOL CHookMgr::Init()
{
	return CBaseHook::HookAll();
}
