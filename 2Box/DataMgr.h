#pragma once
#include "RPCData.h"

class CLock
{
public:
	CLock() { InitializeCriticalSection(&m_csLock); }
	~CLock() { DeleteCriticalSection(&m_csLock); }

	void Lock()	{EnterCriticalSection(&m_csLock);}

	void Unlock(){LeaveCriticalSection(&m_csLock);}
private:
	CRITICAL_SECTION	m_csLock;
};

class CLockGuard
{
public:
	CLockGuard(CLock* pLock)
		: m_pLock(pLock){m_pLock->Lock();}

	~CLockGuard(){m_pLock->Unlock();}
private:
	CLock* m_pLock;
};	

class CWndData
{
public:
	CWndData(EMBED_INFO& data)
		:m_iIndex(-1)
	{
		memcpy(&m_rpc_data,&data,sizeof(m_rpc_data));
	}

	~CWndData(void){}

	EMBED_INFO& GetWndData(){return m_rpc_data;}
	int GetIndex() const {return m_iIndex;}

	void SetIndex(int i){m_iIndex = i;}

private:
	int m_iIndex;
	EMBED_INFO m_rpc_data;
};

typedef Win32xx::Shared_Ptr<CWndData> WndDataPtr;

class CProcessData
{
public:
	CProcessData();
	CProcessData(const NEW_PROC_DATA& data);
	~CProcessData(void);

	NEW_PROC_DATA& GetProcData(){return m_rpc_data;}

	void AddWndData(WndDataPtr WndData);
	void DelWndData( HWND hWnd);
	WndDataPtr FindWndData(HWND hWnd);
	bool IsWndDataEmpty() {return m_set_wnd_data.empty();}
	std::map<HWND,WndDataPtr>& GetAllWndData(){return m_set_wnd_data;}

private:
	NEW_PROC_DATA m_rpc_data;
	std::map<HWND,WndDataPtr> m_set_wnd_data;
};

typedef Win32xx::Shared_Ptr<CProcessData> DataPtr;


class CDataMgr
{
public:
	CDataMgr(void);
	~CDataMgr(void);

	BOOL AddProcessData(DataPtr pData);
	BOOL DelProcessData(int pid);
	DataPtr FindProcessData(int pid);

	void UpdateData(std::vector<int>& WndToDelete);

	BOOL GetAllWnd(std::vector<WndDataPtr>& Wnd);
	BOOL IsEmpty(){CLockGuard g(&m_lockMap);return m_mapProcesses.empty();}

private:
	CLock m_lockMap;
	std::map<int,DataPtr> m_mapProcesses;
};

extern CDataMgr theDataMgr;
