
// ChildView.h : CChildView ��Ľӿ�
//


#pragma once
#include <vector>
#include "DataMgr.h"
#include "HelpWnd.h"

// CChildView ����

class CChildView : public CTabCtrl
{
// ����
public:
	CChildView();

// ����
public:

// ����
public:
	void InsertNewWnd(WndDataPtr WndData);
	void DeleteWnd(int index);
	void HideAllWnd();
	void UpdateAllWnd();

// ��д
protected:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

// ʵ��
public:
	virtual ~CChildView();

	// ���ɵ���Ϣӳ�亯��
protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnKeepRatio();
	afx_msg void OnUpdateKeepRatio(CCmdUI *pCmdUI);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnMouseLeave();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

private:
	void AdjustClient(HWND hWnd,RECT& rcTabClient);
	void AdjustAllCaption();
	BOOL IsCovered(HWND hWnd);
	void KillProc(int iIndex);
	
private:
	CHelpWnd m_wndHelp;
	CFont m_font;
	std::vector<WndDataPtr> m_vecWndData;

	BOOL m_bIsKeepRatio;
	BOOL m_bIsTabPressed;	
};

