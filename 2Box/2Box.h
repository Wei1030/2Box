
// 2Box.h : 2Box Ӧ�ó������ͷ�ļ�
//
#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"       // ������



// CMy2BoxApp:
// �йش����ʵ�֣������ 2Box.cpp
//

class CMy2BoxApp : public CWinApp
{
public:
	CMy2BoxApp();

public:
	void PreDestroyMainWnd();

	UINT GetSettingsInt(LPCTSTR lpszEntry, int nDefault);

	CString GetSettingsString(LPCTSTR lpszEntry, LPCTSTR lpszDefault = NULL);

	BOOL WriteSettingsInt( LPCTSTR lpszEntry, int nValue);

	BOOL WriteSettingsString( LPCTSTR lpszEntry, LPCTSTR lpszValue);

	BOOL GetNewEnv(std::wstring& strOut) const;

	CStringA GetSelfPathA() const {return m_strSelfPathA;}

	CStringW GetSelfPathW() const {return m_strSelfPathW;}

// ��д
public:
	virtual BOOL InitInstance();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	virtual BOOL OnIdle(LONG lCount);	

// ʵ��

public:
	afx_msg void OnAppAbout();
	afx_msg void OnFileOpen();
	DECLARE_MESSAGE_MAP()

private:
	BOOL InitSelfPath();
	//BOOL InitResource();
	//BOOL CreateResource(LPCTSTR lpResName,const CString& FilePath);
	BOOL InitEnv();
	BOOL ProcUnknownEnvStringsW(const void* pszzEnv,
		const std::wstring& loaderPath,
		unsigned long long llData,
		unsigned long lEventNo);
	BOOL CheckInstance();
	BOOL StartLoadFileThread(CString& FileName);	

private:
	static void* SRPC_API NewFileThread( void* usr);
	
private:
	CStringW m_strSelfPathW;
	CStringA m_strSelfPathA;

	void* m_rpcServer;	

	std::wstring m_strEnv;
	size_t m_nLLDataPos;
};

extern CMy2BoxApp theApp;
