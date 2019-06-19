#include "StdAfx.h"
#include "RpcServer.h"
#include "DataMgr.h"
#include "MainFrm.h"

namespace RpcServer
{
	int SRPC_API AcptEvent( void* instance,int type,void* usr )
	{
		if (1 == type)
		{
			theApp.GetMainWnd()->PostMessage(CMainFrame::WM_GAME_UPDATE);
		}
		return 0;
	}

// 	int SRPC_API ConnErr(void* instance,void* usr)
// 	{
// 		theApp.GetMainWnd()->PostMessage(CMainFrame::WM_HELPER_EXCP);
// 		return 0;
// 	}

// 	int MODULE_API HookCompleted( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
// 	{
// 		int iPid = 0;
// 		if (data_in && size_in == sizeof(int))
// 		{
// 			memcpy(&iPid,data_in,size_in);
// 		}
// 		else
// 		{
// 			return -1;
// 		}
// 
// 		DataPtr p = theDataMgr.FindProcessData(iPid);
// 		if (NULL == p.get())
// 		{
// 			return -1;
// 		}
// 
// 		NEW_PROC_DATA& data = p->GetProcData();
// 
// 		if (proc)
// 		{
// 			proc(0,&data,sizeof(NEW_PROC_DATA),usr);
// 		}
// 
// 		return 0;
// 	}

	int MODULE_API OnNewProcess( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
	{
		int iPid = 0;
		if (data_in && size_in == sizeof(int))
		{
			memcpy(&iPid,data_in,size_in);
		}
		else
		{
			return -1;
		}

		NEW_PROC_DATA new_proc_data = {0};
		new_proc_data.iPid = iPid;
		new_proc_data.hNewProcHandleInBox = 
			(unsigned long long)OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_TERMINATE,FALSE,new_proc_data.iPid);
		
		DataPtr pData= new CProcessData(new_proc_data);

		theDataMgr.AddProcessData(pData);

		return 0;
	}

	int MODULE_API OnNewWnd( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
	{
		if (NULL == data_in || size_in != sizeof(EMBED_INFO))
		{
			return -1;
		}

		EMBED_INFO embed_info = {0};
		memcpy(&embed_info,data_in,size_in);

		theApp.GetMainWnd()->SendMessage(CMainFrame::WM_GAME_NEWWND,(WPARAM)&embed_info);

		if (proc)
		{
			proc(0,&embed_info,sizeof(EMBED_INFO),usr);
		}
		return 0;
	}

	int MODULE_API OnEmbedWnd( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
	{
		if (NULL == data_in || size_in != sizeof(EMBED_INFO))
		{
			return -1;
		}

		EMBED_INFO embed_info = {0};
		memcpy(&embed_info,data_in,size_in);

		theApp.GetMainWnd()->SendMessage(CMainFrame::WM_GAME_ATTACH,(WPARAM)&embed_info);

		if (proc)
		{
			proc(0,&embed_info,sizeof(EMBED_INFO),usr);
		}
		return 0;
	}

	int MODULE_API OnActiveWnd( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
	{
		CWnd* pMainWnd = theApp.GetMainWnd();
		if (NULL == pMainWnd)
		{
			return 0;
		}

		if (pMainWnd->GetSafeHwnd())
		{
			//pMainWnd->ShowWindow(SW_SHOWMINIMIZED);
			pMainWnd->ShowWindow(SW_RESTORE);
			pMainWnd->SetWindowPos(&CWnd::wndTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
			pMainWnd->SetWindowPos(&CWnd::wndNoTopMost,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW);
			pMainWnd->SetForegroundWindow();
		}
		return 0;
	}

	int MODULE_API OnGetAllWnd( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
	{
		std::vector<WndDataPtr> vecAllWnd;
		theDataMgr.GetAllWnd(vecAllWnd);
		size_t count = vecAllWnd.size();

		WND_INFO* pWndInfo = new WND_INFO[count + 1]; //加一个主窗口;
		for(size_t i =0;i<count;++i)
		{
			pWndInfo[i].iPid = vecAllWnd[i]->GetWndData().iPid;
			pWndInfo[i].hWnd = vecAllWnd[i]->GetWndData().hWnd;
		}

		pWndInfo[count].iPid = GetCurrentProcessId();
		pWndInfo[count].hWnd = (unsigned long long)theApp.GetMainWnd()->GetSafeHwnd();

		if (proc)
		{
			proc(0,pWndInfo,(count+1) * sizeof(WND_INFO),usr);
		}

		delete[] pWndInfo;
		return 0;
	}

// 	int MODULE_API IsProcessInBox( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
// 	{
// 		int iPid = 0;
// 		if (data_in && size_in == sizeof(int))
// 		{
// 			memcpy(&iPid,data_in,size_in);
// 		}
// 		else
// 		{
// 			return -1;
// 		}
// 
// 		DataPtr p = theDataMgr.FindProcessData(iPid);
// 		if (NULL == p.get())
// 		{
// 			return -1;
// 		}
// 
// 		BOOL b = TRUE;
// 		if (proc)
// 		{
// 			proc(0,&b,sizeof(BOOL),usr);
// 		}
// 
// 		return 0;
// 	}

// 	int MODULE_API InitAllRemoteDlls( const void* data_in, unsigned int size_in, COMPLETIONPROC proc,void* usr )
// 	{
// 		int iPid = 0;
// 		if (data_in && size_in == sizeof(int))
// 		{
// 			memcpy(&iPid,data_in,size_in);
// 		}
// 		else
// 		{
// 			return -1;
// 		}		
// 
// 		if (FALSE == theApp.InitAllDllsByCreateRemoteThread(iPid))
// 		{
// 			return -1;
// 		}
// 
// 		if (proc)
// 		{
// 			BOOL bRet = TRUE;
// 			proc(0,&bRet,sizeof(BOOL),usr);
// 		}
// 		return 0;
// 	}
}