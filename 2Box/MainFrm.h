
// MainFrm.h : CMainFrame ��Ľӿ�
//

#pragma once
#include "ChildView.h"

class CMainFrame : public CFrameWnd
{
	
public:
	CMainFrame();

	enum emMsgOnMainFrame
	{
		WM_GAME_NEWWND = WM_USER+1,
		WM_GAME_ATTACH,
		WM_GAME_DETACH,
		WM_GAME_UPDATE,
		//WM_HELPER_EXCP,
		WM_USER_NOTIFY
	};

	enum emErrOnMainFrame
	{
		ERR_LIB_NOT_FOUND = 1,
		ERR_CREATEPROC_FAIL,
		ERR_DUPLICATEHANDLE_FAIL,
		ERR_INJECT_FAIL
	};

protected: 
	DECLARE_DYNAMIC(CMainFrame)

// ����
public:

// ����
public:
	void CreateFileAndInjectDll(CString& FileName);
	void UpdateViewWhenIdle();

// ��д
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual BOOL OnCmdMsg(UINT nID, int nCode, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo);

// ʵ��
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

	CChildView    m_wndView;

// ���ɵ���Ϣӳ�亯��
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnEnterSizeMove();
	afx_msg void OnSize(UINT nType, int cx, int cy);	
	afx_msg void OnClose();

	LRESULT OnGameNewWnd( WPARAM wParam, LPARAM lParam);
	LRESULT OnGameAttach( WPARAM wParam, LPARAM lParam);
	LRESULT OnGameDetach( WPARAM wParam, LPARAM lParam);
	LRESULT OnGameUpdate( WPARAM wParam, LPARAM lParam);
	//LRESULT OnHelperExcp( WPARAM wParam, LPARAM lParam);
	LRESULT OnUserNotify( WPARAM wParam, LPARAM lParam);
	DECLARE_MESSAGE_MAP()	

private:
	HICON m_hIcon;
	BOOL m_bExit;
	BOOL m_bToTray;
	BOOL m_bIsOnClose;
public:
	afx_msg void OnTotray();
	afx_msg void OnUpdateTotray(CCmdUI *pCmdUI);
};


