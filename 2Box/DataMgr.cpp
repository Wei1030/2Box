#include "StdAfx.h"
#include "DataMgr.h"
#include "2Box.h"

CProcessData::CProcessData()
{
	memset(&m_rpc_data,0,sizeof(NEW_PROC_DATA));
}

CProcessData::CProcessData( const NEW_PROC_DATA& data)
{
	memcpy(&m_rpc_data,&data,sizeof(NEW_PROC_DATA));
}

CProcessData::~CProcessData(void)
{
	if (m_rpc_data.hNewProcHandleInBox)
	{
		TerminateProcess((HANDLE)m_rpc_data.hNewProcHandleInBox,0);
		CloseHandle((HANDLE)m_rpc_data.hNewProcHandleInBox);

		theApp.StartFileSysCleanup(m_rpc_data.iPid);
	}
}

void CProcessData::AddWndData( WndDataPtr WndData )
{
	if (NULL == WndData.get())
	{
		return;
	}
	m_set_wnd_data.insert(std::make_pair((HWND)WndData->GetWndData().hWnd,WndData));
}

void CProcessData::DelWndData( HWND hWnd)
{
	m_set_wnd_data.erase(hWnd);
}

WndDataPtr CProcessData::FindWndData( HWND hWnd)
{
	std::map<HWND,WndDataPtr>::const_iterator it = m_set_wnd_data.find(hWnd);
	if (it == m_set_wnd_data.end())
	{
		return NULL;
	}
	return it->second;
}

CDataMgr theDataMgr;

CDataMgr::CDataMgr(void)
{
}

CDataMgr::~CDataMgr(void)
{
}

BOOL CDataMgr::AddProcessData( DataPtr pData )
{
	CLockGuard g(&m_lockMap);
	m_mapProcesses.insert(std::make_pair(pData->GetProcData().iPid,pData));
	return TRUE;			

}

BOOL CDataMgr::DelProcessData( int pid )
{
	CLockGuard g(&m_lockMap);
	m_mapProcesses.erase(pid);
	return TRUE;
}

DataPtr CDataMgr::FindProcessData( int pid )
{
	DataPtr p;
	CLockGuard g(&m_lockMap);
	std::map<int ,DataPtr>::const_iterator it = m_mapProcesses.find(pid);
	if (it != m_mapProcesses.end())
	{
		p = it->second;
	}
	return p;
}

void CDataMgr::UpdateData(std::vector<int>& WndToDelete)
{
	DWORD dwExitCode = 0;
	std::map<HWND,WndDataPtr>::const_iterator itWnd;

	CLockGuard g(&m_lockMap);
	std::map<int ,DataPtr>::const_iterator it = m_mapProcesses.begin();
	while (it != m_mapProcesses.end())
	{
		if (GetExitCodeProcess((HANDLE)it->second->GetProcData().hNewProcHandleInBox,&dwExitCode))
		{
			if (dwExitCode == STILL_ACTIVE)
			{
				it++;
				continue;
			}
		}

		if (false == it->second->IsWndDataEmpty())
		{
			std::map<HWND,WndDataPtr>& tmp = it->second->GetAllWndData();
			for (itWnd = tmp.begin();
				itWnd != tmp.end();
				++itWnd)
			{
				WndToDelete.push_back(itWnd->second->GetIndex());
			}
		}

		it = m_mapProcesses.erase(it);
	}
}

BOOL CDataMgr::GetAllWnd( std::vector<WndDataPtr>& Wnd )
{
	std::map<HWND,WndDataPtr>::const_iterator itWnd;

	CLockGuard g(&m_lockMap);
	std::map<int ,DataPtr>::const_iterator it = m_mapProcesses.begin();
	while (it != m_mapProcesses.end())
	{
		if (false == it->second->IsWndDataEmpty())
		{
			std::map<HWND,WndDataPtr>& tmp = it->second->GetAllWndData();
			for (itWnd = tmp.begin();
				itWnd != tmp.end();
				++itWnd)
			{
				Wnd.push_back(itWnd->second);
			}
		}
		++it;
	}

	return TRUE;
}
