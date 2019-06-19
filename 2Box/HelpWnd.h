#pragma once


// CHelpWnd

class CHelpWnd : public CWnd
{
	DECLARE_DYNAMIC(CHelpWnd)

public:
	CHelpWnd();
	virtual ~CHelpWnd();

	void SetShowText(BOOL bShow = TRUE){m_bShowText = bShow;}

protected:
	afx_msg void OnPaint();
	DECLARE_MESSAGE_MAP()
private:
	BOOL m_bShowText;
	CFont m_font;
	CString m_strNote;
};


