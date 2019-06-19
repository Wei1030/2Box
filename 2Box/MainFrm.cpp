
// MainFrm.cpp : CMainFrame ���ʵ��
//

#include "stdafx.h"
#include "2Box.h"

#include "MainFrm.h"
#include "DataMgr.h"
#include "detours.h"
#include "Job.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

// CMainFrame ����/����

CMainFrame::CMainFrame() 
: m_hIcon(NULL)
, m_bExit(FALSE)
, m_bToTray(FALSE)
, m_bIsOnClose(FALSE)
{
	// TODO: �ڴ���ӳ�Ա��ʼ������;
	m_bToTray = theApp.GetSettingsInt(_T("ToTray"),TRUE);
}

CMainFrame::~CMainFrame()
{
}

void CMainFrame::CreateFileAndInjectDll( CString& FileName )
{	
	CJob job;
	if (FALSE == job.Create())
	{
		PostMessage(WM_GAME_DETACH,-1);
		return;
	}

	CString strCurDir = FileName.Left(FileName.ReverseFind('\\')+1);	

	TCHAR szCmd[MAX_PATH+1] = {0};
	_stprintf_s(szCmd,MAX_PATH,_T("\"%s\""),(const TCHAR*)FileName);
	
	CStringA strDllPath = theApp.GetSelfPathA();
	strDllPath += "2BoxMonitor32.dll";

	PROCESS_INFORMATION ProcInfo	= {0};  
	STARTUPINFO StartupInfo			= {0};
	StartupInfo.cb = sizeof(StartupInfo);
	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
	StartupInfo.wShowWindow = SW_SHOW;

	std::wstring strEnv;
	if (FALSE == theApp.GetNewEnv(strEnv))
	{
		PostMessage(WM_GAME_DETACH,-1);
		return;
	}

	if(!DetourCreateProcessWithDllEx( NULL, szCmd,NULL,NULL, FALSE,  
		CREATE_DEFAULT_ERROR_MODE | CREATE_BREAKAWAY_FROM_JOB | CREATE_UNICODE_ENVIRONMENT | CREATE_SUSPENDED ,
		(void*)strEnv.c_str(),(const TCHAR*)strCurDir,
		&StartupInfo,&ProcInfo,
		(const char*)strDllPath,NULL))
	{
		PostMessage(WM_GAME_DETACH,ERR_CREATEPROC_FAIL);	
		return;
	}
	
	DataPtr pData= new CProcessData;
	NEW_PROC_DATA&  new_proc_data = pData->GetProcData();

	new_proc_data.iPid = ProcInfo.dwProcessId;
	new_proc_data.hNewProcHandleInBox = (unsigned long long)ProcInfo.hProcess;

	theDataMgr.AddProcessData(pData);

	int iResult = -1;

	do 
	{
		if (FALSE == job.AssignProcess(ProcInfo.hProcess))
		{
			break;
		}

// 		if (FALSE == job.SetBasicUIRestrictions( JOB_OBJECT_UILIMIT_ALL ))
// 		{
// 			break;
// 		}
		
		iResult = 0;
	} while (0);


	if (0 == iResult)
	{
		ResumeThread(ProcInfo.hThread);
	}
	else
	{
		theDataMgr.DelProcessData(ProcInfo.dwProcessId);//also terminate the process and close the process handle
		PostMessage(WM_GAME_DETACH,iResult);
	}

	CloseHandle(ProcInfo.hThread);		
	return;
}

void CMainFrame::UpdateViewWhenIdle()
{
	if (m_wndView.GetSafeHwnd())
	{
		m_wndView.UpdateAllWnd();
	}
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: �ڴ˴�ͨ���޸�
	//  CREATESTRUCT cs ���޸Ĵ��������ʽ

 	cs.dwExStyle &= ~WS_EX_CLIENTEDGE;
 	//cs.lpszClass = AfxRegisterWndClass(0);
	cs.cx = 640;
	cs.cy = 480;
	return TRUE;
}

BOOL CMainFrame::OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo)
{
	// TODO: �ڴ����ר�ô����/����û���
	// ����ͼ��һ�γ��Ը�����
	if (m_wndView.OnCmdMsg(nID, nCode, pExtra, pHandlerInfo))
		return TRUE;

	return CFrameWnd::OnCmdMsg(nID, nCode, pExtra, pHandlerInfo);
}

// CMainFrame ���

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CFrameWnd::Dump(dc);
}
#endif //_DEBUG


BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	ON_MESSAGE(WM_GAME_NEWWND,&OnGameNewWnd)
	ON_MESSAGE(WM_GAME_ATTACH,&OnGameAttach)
	ON_MESSAGE(WM_GAME_DETACH,&OnGameDetach)
	ON_MESSAGE(WM_GAME_UPDATE,&OnGameUpdate)
	//ON_MESSAGE(WM_HELPER_EXCP,&OnHelperExcp)
	ON_MESSAGE(WM_USER_NOTIFY,&OnUserNotify)

	//////////////////////////////////////////////////////////////////////////
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ENTERSIZEMOVE()
	ON_WM_SIZE()		
	ON_WM_CLOSE()
	ON_COMMAND(IDM_TOTRAY, &CMainFrame::OnTotray)
	ON_UPDATE_COMMAND_UI(IDM_TOTRAY, &CMainFrame::OnUpdateTotray)
END_MESSAGE_MAP()


// CMainFrame ��Ϣ�������
LRESULT CMainFrame::OnGameNewWnd( WPARAM wParam, LPARAM lParam )
{
	EMBED_INFO* pEmbedInfo = (EMBED_INFO*)wParam;
	if (NULL == pEmbedInfo)
	{
		return 0;
	}

	DataPtr p = theDataMgr.FindProcessData(pEmbedInfo->iPid);
	if (NULL == p.get())
	{
		return 0;
	}

	WndDataPtr pWndData = new CWndData(*pEmbedInfo);
	p->AddWndData(pWndData);

	return 0;
}

LRESULT CMainFrame::OnGameAttach( WPARAM wParam, LPARAM lParam )
{
	EMBED_INFO* pEmbedInfo = (EMBED_INFO*)wParam;
	if (NULL == pEmbedInfo)
	{
		return 0;
	}

	DataPtr p = theDataMgr.FindProcessData(pEmbedInfo->iPid);
	if (NULL == p.get())
	{
		return 0;
	}

	WndDataPtr pWndData = p->FindWndData((HWND)pEmbedInfo->hWnd);
	if (NULL == pWndData.get() || pWndData->GetIndex() != -1)
	{
		return 0;
	}

	memcpy(pWndData->GetWndData().szTitle,pEmbedInfo->szTitle,260);

	m_wndView.InsertNewWnd(pWndData);

	return 0;
}

LRESULT CMainFrame::OnGameDetach( WPARAM wParam, LPARAM lParam )
{
	switch(wParam)
	{
	case ERR_LIB_NOT_FOUND:
		MessageBox(_T("�Ҳ���2Box_img_loader.dll,�뽫�����ڱ������Ŀ¼.\r\n")
			_T("����ʧ���ļ�,����������\r\n")
			_T("���ص�ַ��\"����\"-\"����\""),_T("����"),MB_OK);
		break;
	case ERR_CREATEPROC_FAIL:
		MessageBox(_T("��������ʧ��"),_T("����"),MB_OK);
		break;
	case ERR_DUPLICATEHANDLE_FAIL:
		MessageBox(_T("�������ʧ��,����ʹ�ù���Ա���б�����"),_T("����"),MB_OK);
		break;
	case ERR_INJECT_FAIL:
		MessageBox(_T("ע�����ʧ��"),_T("����"),MB_OK);
		break;
	default:
		MessageBox(_T("δ֪"),_T("����"),MB_OK);
		break;
	}

	return 0;
}

LRESULT CMainFrame::OnGameUpdate( WPARAM wParam, LPARAM lParam )
{
	std::vector<int> WndToDelete;
	std::vector<int>::const_iterator it;

	theDataMgr.UpdateData(WndToDelete);

	for (it = WndToDelete.begin();
		it != WndToDelete.end();
		++it)
	{
		m_wndView.DeleteWnd((*it));
	}

	return 0;
}

LRESULT CMainFrame::OnUserNotify(WPARAM wParam, LPARAM lParam)
{
	switch(lParam)
	{
	case WM_LBUTTONUP:
		//ShowWindow(SW_SHOWMINIMIZED);
		ShowWindow(SW_RESTORE);
		SetForegroundWindow();
		break;
	case WM_RBUTTONUP:
		{
			CMenu menu;
			if (FALSE == menu.LoadMenu(IDR_MAINFRAME))
			{
				break;
			}

			CMenu *pPopMenu = menu.GetSubMenu(0);
			if (NULL == pPopMenu)
			{
				break;
			}

			CPoint screenPoint;
			GetCursorPos(&screenPoint);
			pPopMenu->TrackPopupMenuEx(TPM_LEFTALIGN,screenPoint.x,screenPoint.y,this,NULL);
		}
		break;
	default:
		break;
	}
	return 0;
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	//���ñ�������ͼ��
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	SetIcon(m_hIcon,TRUE);
	SetIcon(m_hIcon,FALSE);

	// ����һ����ͼ��ռ�ÿ�ܵĹ�����
	if (!m_wndView.Create(WS_CHILD|WS_VISIBLE|TCS_FIXEDWIDTH|TCS_FORCELABELLEFT,
		CRect(0, 0, 0, 0), this, 1))
	{
		TRACE0("δ�ܴ�����ͼ����\n");
		return -1;
	}

	TCHAR sz[5] = {'2','B','o','x'};
	SetWindowText(sz);

	if (m_bToTray)
	{
		NOTIFYICONDATA nid = {0};
		nid.cbSize	=	sizeof(nid);
		nid.hWnd	= m_hWnd;
		nid.uID		= IDR_MAINFRAME;
		nid.uFlags	= NIF_ICON | NIF_MESSAGE;
		nid.uCallbackMessage = WM_USER_NOTIFY;
		nid.hIcon	= m_hIcon;

		Shell_NotifyIcon(NIM_ADD,&nid);
	}
	return 0;
}

void CMainFrame::OnDestroy()
{
	m_bExit = TRUE;
	theApp.PreDestroyMainWnd();
	CFrameWnd::OnDestroy();

	// TODO: �ڴ˴������Ϣ����������
}

void CMainFrame::OnEnterSizeMove()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (m_wndView.GetSafeHwnd())
	{
		m_wndView.HideAllWnd();
	}

	CFrameWnd::OnEnterSizeMove();
}

void CMainFrame::OnSize(UINT nType, int cx, int cy)
{
	CFrameWnd::OnSize(nType, cx, cy);

	// TODO: �ڴ˴������Ϣ����������
	if (SIZE_MINIMIZED == nType)
	{
		m_wndView.HideAllWnd();
		if (m_bToTray)
		{
			ShowWindow(SW_HIDE);
		}
		return;
	}

	if (m_wndView.GetSafeHwnd())
	{
		RECT rc = {0};
		GetClientRect(&rc);

		m_wndView.MoveWindow(&rc,TRUE);
	}	
}

void CMainFrame::OnClose()
{
	// TODO: �ڴ������Ϣ�����������/�����Ĭ��ֵ;
	if (m_bIsOnClose)
	{
		return;
	}

	m_bIsOnClose = TRUE;

	if (FALSE == theDataMgr.IsEmpty())
	{
		if (IDCANCEL == MessageBox(L"�رձ��������ر������Ѿ��򿪵ĳ���ȷ���ر���",
			L"ע��",MB_OKCANCEL) )
		{
			m_bIsOnClose = FALSE;
			return;
		}		
	}

	NOTIFYICONDATA nid = {0};
	nid.cbSize	= sizeof(nid);
	nid.hWnd	= m_hWnd;
	nid.uID		= IDR_MAINFRAME;
	Shell_NotifyIcon(NIM_DELETE,&nid);
	
	CFrameWnd::OnClose();

	m_bIsOnClose = FALSE;
}

void CMainFrame::OnTotray()
{
	// TODO: �ڴ���������������;
	m_bToTray = !m_bToTray;
	theApp.WriteSettingsInt(_T("ToTray"),m_bToTray);

	if (m_bToTray)
	{
		NOTIFYICONDATA nid = {0};
		nid.cbSize	=	sizeof(nid);
		nid.hWnd	= m_hWnd;
		nid.uID		= IDR_MAINFRAME;
		nid.uFlags	= NIF_ICON | NIF_MESSAGE;
		nid.uCallbackMessage = WM_USER_NOTIFY;
		nid.hIcon	= m_hIcon;

		Shell_NotifyIcon(NIM_ADD,&nid);
	}
	else
	{
		NOTIFYICONDATA nid = {0};
		nid.cbSize	= sizeof(nid);
		nid.hWnd	= m_hWnd;
		nid.uID		= IDR_MAINFRAME;
		Shell_NotifyIcon(NIM_DELETE,&nid);
	}
}

void CMainFrame::OnUpdateTotray(CCmdUI *pCmdUI)
{
	// TODO: �ڴ������������û����洦��������;
	pCmdUI->SetCheck(m_bToTray);
}
