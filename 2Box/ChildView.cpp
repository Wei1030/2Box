
// ChildView.cpp : CChildView 类的实现
//

#include "stdafx.h"
#include "2Box.h"
#include "ChildView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CChildView

CChildView::CChildView()
: m_bIsKeepRatio(TRUE)
, m_bIsTabPressed(FALSE)
{
	m_font.CreatePointFont(11 * 9, _T("微软雅黑"));

	m_bIsKeepRatio = theApp.GetSettingsInt(_T("KeepRatio"),TRUE);
}

CChildView::~CChildView()
{
}

void CChildView::InsertNewWnd( WndDataPtr WndData )
{
	if (WndData->GetIndex() != -1)
	{
		return;
	}

	if (m_vecWndData.size() != GetItemCount())
	{
		MessageBox(_T("窗口数目有误!"),_T("未知错误"),MB_OK);
		return;
	}

	//这句一定要先执行,这样函数入口判断Index不为-1才有意义,原因见下面的ShowWindow;
	WndData->SetIndex(m_vecWndData.size());

	DWORD dwExStyleChange = WndData->GetWndData().dwExStyle;
	dwExStyleChange &= ~WS_EX_APPWINDOW;
	dwExStyleChange |= WS_EX_TOOLWINDOW;

	//ShowWindow操作一个其他进程的窗口时,函数内部为了不卡死,会直接处理其他消息;
	//这个InsertNewWnd函数在WM_GAME_ATTACH消息处理里调用
	//所以这里要注意ShowWindow内部可能会接着调用InsertNewWnd.
	::ShowWindow((HWND)WndData->GetWndData().hWnd,SW_HIDE);

	::SetWindowLongPtr((HWND)WndData->GetWndData().hWnd,GWL_EXSTYLE,dwExStyleChange);
	if (FALSE == m_bIsKeepRatio)
	{
		::SetWindowLongPtr((HWND)WndData->GetWndData().hWnd,GWL_STYLE,WS_POPUP);
	}	

	m_vecWndData.push_back(WndData);	
	InsertItem(GetItemCount(),WndData->GetWndData().szTitle);

	SetCurSel(GetItemCount()-1);

	UpdateAllWnd();

	m_wndHelp.SetShowText(FALSE);
}

void CChildView::DeleteWnd( int index )
{
	if (index <0 || index >= GetItemCount())
	{
		return;
	}

	int iPos = 0;
	std::vector<WndDataPtr>::const_iterator it;
	for (it = m_vecWndData.begin();
		it!=m_vecWndData.end();
		it++)
	{
		if ((*it)->GetIndex() == index)
		{
			(*it)->SetIndex(-1);
			it = m_vecWndData.erase(it);
			DeleteItem(index);
			break;
		}
	}

	for (iPos=index;it!=m_vecWndData.end();it++,iPos++)
	{
		(*it)->SetIndex(iPos);
	}

	if (0 == GetItemCount())
	{
		m_wndHelp.SetShowText();
		m_wndHelp.Invalidate();
	}
}

void CChildView::HideAllWnd()
{
	std::vector<WndDataPtr>::const_iterator it;
	for (it = m_vecWndData.begin();
		it != m_vecWndData.end();
		++it)
	{
		::SetWindowPos((HWND)(*it)->GetWndData().hWnd,NULL,
			0,0,0,0,
			SWP_ASYNCWINDOWPOS|SWP_HIDEWINDOW|SWP_NOACTIVATE|
			SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
	}
}

void CChildView::UpdateAllWnd()
{
	if (m_bIsTabPressed)
	{
		return;
	}

	int i = GetCurSel();

	if (i <0 )
	{
		return;
	}

	std::vector<WndDataPtr>::const_iterator it;
	for (it = m_vecWndData.begin();
		it != m_vecWndData.end();
		++it)
	{
		if (FALSE == IsIconic() && (*it)->GetIndex() == i)
		{
			if (IsCovered((HWND)(*it)->GetWndData().hWnd))
			{
				continue;
			}

			RECT rc		= {0};
			RECT rcItem = {0};
			UINT uFlags = SWP_SHOWWINDOW;

			if (m_bIsKeepRatio)
			{
				uFlags |= SWP_NOSIZE;
			}

			GetClientRect(&rc);
			GetItemRect(i,&rcItem);
			rc.top += rcItem.bottom;

			ClientToScreen(&rc);

			::SetWindowPos((HWND)(*it)->GetWndData().hWnd,HWND_TOP,
				rc.left,rc.top,
				rc.right-rc.left,
				rc.bottom-rc.top,
				uFlags);

			AdjustClient((HWND)(*it)->GetWndData().hWnd,rc);
		}
		else
		{
			::SetWindowPos((HWND)(*it)->GetWndData().hWnd,NULL,
				0,0,0,0,
				SWP_ASYNCWINDOWPOS|SWP_HIDEWINDOW|SWP_NOACTIVATE|
				SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
		}
	}	
}

void CChildView::AdjustClient( HWND hWnd,RECT& rcTabClient )
{
	if (FALSE == m_bIsKeepRatio || FALSE == IsWindow(hWnd))
	{
		return;
	}

	CRect rcGame;
	if (FALSE == ::GetWindowRect(hWnd,&rcGame))
	{
		return;
	}

	RECT rcMain;
	theApp.GetMainWnd()->GetWindowRect(&rcMain);

	int offsetL = rcTabClient.left	- rcMain.left;
	int offsetR = rcMain.right		- rcTabClient.right;
	int offsetT = rcTabClient.top	- rcMain.top;
	int offsetB = rcMain.bottom		- rcTabClient.bottom;

	rcGame.left		-= offsetL;
	rcGame.right	+= offsetR;
	rcGame.top		-= offsetT;
	rcGame.bottom	+= offsetB;

	if (rcGame.Width() >= 10 && rcGame.Height() >= 10)
	{
		theApp.GetMainWnd()->MoveWindow(&rcGame);
	}
}

void CChildView::AdjustAllCaption()
{
	std::vector<WndDataPtr>::const_iterator it;
	for (it = m_vecWndData.begin();
		it != m_vecWndData.end();
		++it)
	{
		::ShowWindow((HWND)(*it)->GetWndData().hWnd,SW_HIDE);

		if (m_bIsKeepRatio)
		{
			::SetWindowLongPtr((HWND)(*it)->GetWndData().hWnd,GWL_STYLE,(*it)->GetWndData().dwStyle);
		}
		else
		{
			::SetWindowLongPtr((HWND)(*it)->GetWndData().hWnd,GWL_STYLE,WS_POPUP);
		}		
	}
}

BOOL CChildView::IsCovered( HWND hWnd )
{
	BOOL bCovered	= FALSE;
	HWND hAboveWnd	= ::GetNextWindow(theApp.GetMainWnd()->GetSafeHwnd(),GW_HWNDPREV); 
	
	for (;
		hAboveWnd != NULL;
		hAboveWnd = ::GetNextWindow(hAboveWnd,GW_HWNDPREV))
	{
		if (FALSE == ::IsWindowVisible(hAboveWnd))
		{
			continue;
		}

		if (hAboveWnd == hWnd)
		{
			bCovered = TRUE;
			break;
		}
	}

	return bCovered;
}

void CChildView::KillProc( int index )
{
	if (index <0 || index >= GetItemCount())
	{
		return;
	}

	WndDataPtr p = m_vecWndData[index];
	if (NULL == p.get())
	{
		return;
	}

	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE,FALSE,p->GetWndData().iPid);
	if (NULL == hProcess)
	{
		return;
	}

	TerminateProcess(hProcess,0);
	CloseHandle(hProcess);
}

BEGIN_MESSAGE_MAP(CChildView, CTabCtrl)
	ON_WM_CREATE()
	ON_COMMAND(IDM_KEEP_RATIO, &CChildView::OnKeepRatio)
	ON_UPDATE_COMMAND_UI(IDM_KEEP_RATIO, &CChildView::OnUpdateKeepRatio)	
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSELEAVE()
	ON_WM_RBUTTONUP()
	ON_WM_MBUTTONUP()
	ON_WM_SIZE()
END_MESSAGE_MAP()



// CChildView 消息处理程序

BOOL CChildView::PreCreateWindow(CREATESTRUCT& cs) 
{
	if (!CTabCtrl::PreCreateWindow(cs))
		return FALSE;

	//cs.dwExStyle |= WS_EX_CLIENTEDGE;
	cs.style &= ~WS_BORDER;

	return TRUE;
}

int CChildView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CTabCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	// TODO:  在此添加您专用的创建代码

	SetFont(&m_font,TRUE);

	if (!m_wndHelp.Create(NULL,NULL,WS_CHILD | WS_VISIBLE,CRect(0,0,0,0),this,AFX_IDW_PANE_FIRST))
	{
		return -1;
	}

	return 0;
}

void CChildView::OnKeepRatio()
{
	// TODO: 在此添加命令处理程序代码
	m_bIsKeepRatio = !m_bIsKeepRatio;
	theApp.WriteSettingsInt(_T("KeepRatio"),m_bIsKeepRatio);
	AdjustAllCaption();
}

void CChildView::OnUpdateKeepRatio(CCmdUI *pCmdUI)
{
	// TODO: 在此添加命令更新用户界面处理程序代码
	pCmdUI->SetCheck(m_bIsKeepRatio);
}

void CChildView::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bIsTabPressed = TRUE;
	CTabCtrl::OnLButtonDown(nFlags, point);
}

void CChildView::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_bIsTabPressed = FALSE;
	CTabCtrl::OnLButtonUp(nFlags, point);
}

void CChildView::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (FALSE == m_bIsTabPressed)
	{
		return CTabCtrl::OnMouseMove(nFlags, point);
	}

	int iCurSel	= GetCurSel();
	int i		= 0;
	BOOL bInTab = FALSE;
	CRect rcItem;
	
	for (i = 0;i < GetItemCount(); ++i)
	{
		GetItemRect(i,&rcItem);
		if (rcItem.PtInRect(point))
		{
			bInTab = TRUE;
			break;
		}
	}

	if (bInTab && i != iCurSel)
	{
		WndDataPtr p	= m_vecWndData[i];
		WndDataPtr pCur = m_vecWndData[iCurSel];

		p->SetIndex(iCurSel);
		pCur->SetIndex(i);

		m_vecWndData[i]			= pCur;
		m_vecWndData[iCurSel]	= p;

		TCITEM tc	= {0};
		tc.mask		= TCIF_TEXT;
		tc.cchTextMax = 260;
		tc.pszText	= pCur->GetWndData().szTitle;
		SetItem(i,&tc);

		tc.pszText	= p->GetWndData().szTitle;
		SetItem(iCurSel,&tc);

		SetCurSel(i);
	}	

	CTabCtrl::OnMouseMove(nFlags, point);
}

void CChildView::OnMouseLeave()
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (m_bIsTabPressed)
	{
		m_bIsTabPressed = FALSE;

		int iCurSel = GetCurSel();
		if (iCurSel >= 0)
		{
			WndDataPtr pCur = m_vecWndData[iCurSel];

			::ShowWindow((HWND)pCur->GetWndData().hWnd,SW_HIDE);

			::SetWindowLongPtr((HWND)pCur->GetWndData().hWnd,GWL_EXSTYLE,pCur->GetWndData().dwExStyle);
			::SetWindowLongPtr((HWND)pCur->GetWndData().hWnd,GWL_STYLE,pCur->GetWndData().dwStyle);

			POINT pt;
			GetCursorPos(&pt);

			::SetWindowPos((HWND)pCur->GetWndData().hWnd,HWND_TOP,
				pt.x - 50,pt.y - 10,0,0,
				SWP_NOSIZE | SWP_SHOWWINDOW);

			DeleteWnd(iCurSel);

			int x = pt.x * 65535 / GetSystemMetrics(SM_CXSCREEN);
			int y = pt.y * 65535 / GetSystemMetrics(SM_CYSCREEN);

			mouse_event(MOUSEEVENTF_LEFTUP,0,0,0,GetMessageExtraInfo());
			Sleep(100);
			mouse_event(MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE | MOUSEEVENTF_LEFTDOWN ,x,y,0,GetMessageExtraInfo());

		}
	}

	CTabCtrl::OnMouseLeave();
}

void CChildView::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CMenu menu;
	if (FALSE == menu.LoadMenu(IDR_MENU_TABRCLICK))
	{
		return;
	}

	CMenu *pPopMenu = menu.GetSubMenu(0);
	if (NULL == pPopMenu)
	{
		return;
	}

	CPoint screenPoint = point;
	ClientToScreen(&screenPoint);
	LONG lRes = pPopMenu->TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN,screenPoint.x,screenPoint.y,this);
	if (ID_TAB_CLOSE == lRes)
	{
		CRect rcItem;
		for (int i = 0;i < GetItemCount(); ++i)
		{
			GetItemRect(i,&rcItem);
			if (rcItem.PtInRect(point))
			{
				KillProc(i);
				break;
			}
		}
	}	

	CTabCtrl::OnRButtonUp(nFlags, point);
}

void CChildView::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	CRect rcItem;
	for (int i = 0;i < GetItemCount(); ++i)
	{
		GetItemRect(i,&rcItem);
		if (rcItem.PtInRect(point))
		{
			KillProc(i);
			break;
		}
	}
	CTabCtrl::OnMButtonUp(nFlags, point);
}

void CChildView::OnSize(UINT nType, int cx, int cy)
{
	CTabCtrl::OnSize(nType, cx, cy);

	// TODO: 在此处添加消息处理程序代码
	int nH	= 0;
	RECT rc = {0};
	GetItemRect(0,&rc);
	nH = rc.bottom;

	if (nH < cy)
	{
		m_wndHelp.MoveWindow(0,nH,cx,cy-nH);
	}
	else
	{
		m_wndHelp.MoveWindow(0,0,0,0);
	}
}
