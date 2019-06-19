// HelpWnd.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "2Box.h"
#include "HelpWnd.h"


// CHelpWnd

IMPLEMENT_DYNAMIC(CHelpWnd, CWnd)

CHelpWnd::CHelpWnd()
: m_bShowText(TRUE)
, m_strNote(_T(""))
{
	m_font.CreatePointFont(11 * 9, _T("΢���ź�"));
	TCHAR pt = L' ';
	CString strTemp;

	m_strNote += _T("2Box\r\n��������:\r\n");
	TCHAR msgNote[512] = {
		_T('��'),_T('��'),_T('��'),_T('��'),_T('ȫ'),_T('��'),_T('��'),',',
		_T('��'),_T('��'),_T('ѧ'),_T('ϰ'),_T('��'),_T('��'),',','\r','\n',
		_T('��'),_T('��'),_T('��'),_T('��'),_T('��'),_T('��'),_T('��'),_T('ҵ'),_T('��'),_T(';'),',',
		_T('��'),_T('��'),_T('��'),_T('��'),_T('��'),_T('��'),'!','\r','\n'
	};

	m_strNote += msgNote;
	m_strNote += _T("\r\n\r\n");

	m_strNote += _T("��Ѹ��¼����ص�ַ��\"����\"--\"˵��...\"\r\n\r\n");

	m_strNote += _T("ʹ��˵��:\r\n");	
	
	strTemp = _T("1���ر������������е�*��Ҫ�࿪�ĳ���*");
	while(strTemp.GetLength() < 26){strTemp.AppendChar(pt);}
	strTemp.Append(L"\r\n");
	m_strNote += strTemp;
	
	strTemp = _T("2����*������*�Ϸ��˵������\"�ļ�\"-\"��...\"");
	while(strTemp.GetLength() < 0){strTemp.AppendChar(pt);}
	strTemp.Append(L"\r\n");
	m_strNote += strTemp;
	
	strTemp = _T("3���ڵ����ĶԻ�����ѡ����Ҫ�࿪�ĳ���");
	while(strTemp.GetLength() < 25){strTemp.AppendChar(pt);}
	strTemp.Append(L"\r\n");
	m_strNote += strTemp;
	
	strTemp = _T("4���ظ���2-3���輴��");
	while(strTemp.GetLength() < 45){strTemp.AppendChar(pt);}
	strTemp.Append(L"\r\n");
	m_strNote += strTemp;

	m_strNote += _T("\r\n******��ѡ����******\r\n");	
	m_strNote += _T("��*�򿪵ĳ���*��[������]�Ҽ�����[������ͼ��](Shift +)�Ҽ�\r\n");	
	m_strNote += _T("�ڵ�����ϵͳ�˵���,��ѡ��\r\n");
	
	//m_strNote += _T("a���ر����п��ɾ��(*��*���ظ���3��*�࿪ʧ��ʱ*ʹ�ã��ù��ܿ��ܵ��³����ȶ���)\r\n");
	
	strTemp = _T("a������box����(���Ա�ǩҳ��ʽ����࿪�ĳ��򴰿ڣ�����ǩҳ�ϳ����ɸ�ԭ)");
	while(strTemp.GetLength() < 53){strTemp.AppendChar(pt);}
	strTemp.Append(L"\r\n");
	m_strNote += strTemp;
}

CHelpWnd::~CHelpWnd()
{
}


BEGIN_MESSAGE_MAP(CHelpWnd, CWnd)
	ON_WM_PAINT()
END_MESSAGE_MAP()



// CHelpWnd ��Ϣ�������



void CHelpWnd::OnPaint()
{
	CPaintDC dc(this); // device context for painting
	if (m_bShowText)
	{
		CRect rc;
		GetClientRect(&rc);
		rc.DeflateRect(10,10);
		HGDIOBJ old = dc.SelectObject(m_font);
		dc.DrawText(m_strNote,&rc,DT_CENTER | DT_VCENTER);
		dc.SelectObject(old);
	}
	CWnd::OnPaint();
}
