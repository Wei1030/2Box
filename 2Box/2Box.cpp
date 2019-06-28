
// 2Box.cpp : 定义应用程序的类行为。
//

#include "stdafx.h"
#include "afxwinappex.h"
#include "2Box.h"
#include "MainFrm.h"
#include "RpcServer.h"

#include <sys/timeb.h>

#pragma comment (lib, "sRPC.lib")

int SRPC_API bResultCallBack(unsigned int error,const void* data,unsigned int size,void* usr)
{
	if (error || NULL == data || size != sizeof(BOOL) || NULL == usr)
	{
		return -1;
	}

	memcpy(usr,data,size);
	return 0;
}

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()

private:
	CString m_strNote;
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
, m_strNote(_T(""))
{
	m_strNote += _T("免责声明:\r\n");
	TCHAR msgNote[512] = {
		_T('本'),_T('软'),_T('件'),_T('完'),_T('全'),_T('免'),_T('费'),',',
		_T('仅'),_T('供'),_T('学'),_T('习'),_T('交'),_T('流'),',','\r','\n',
		_T('请'),_T('勿'),_T('用'),_T('于'),_T('任'),_T('何'),_T('商'),_T('业'),_T('用'),_T('途'),',','\r','\n',
		_T('否'),_T('则'),_T('后'),_T('果'),_T('自'),_T('负'),'!','\r','\n'
	};

	m_strNote += msgNote;
	m_strNote += _T("\r\n\r\n");

	m_strNote += _T("免费下载地址:\r\n");
	TCHAR msgAddr[512] = {
		_T('链'),_T('接'),':','h','t','t','p','s',':','/','/','p','a','n',
		'.','b','a','i','d','u','.','c','o','m','/','s','/',
		'1','b','p','Y','J','4','w','Z','\r','\n',
		_T('密'),_T('码'),':','a','h','h','4'
	};

	m_strNote += msgAddr;
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_TIP, m_strNote);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()
















// CMy2BoxApp

BEGIN_MESSAGE_MAP(CMy2BoxApp, CWinApp)
	ON_COMMAND(ID_APP_ABOUT, &CMy2BoxApp::OnAppAbout)
	ON_COMMAND(ID_FILE_OPEN, &CMy2BoxApp::OnFileOpen)
END_MESSAGE_MAP()


void* SRPC_API CMy2BoxApp::NewFileThread( void* usr )
{
	CString* pstrName = (CString*)usr;
	if (NULL == pstrName)
	{
		return 0;
	}

	((CMainFrame*)theApp.GetMainWnd())->CreateFileAndInjectDll(*pstrName);

	delete pstrName;
	return 0;
}

// CMy2BoxApp 构造

CMy2BoxApp::CMy2BoxApp()
: m_rpcServer(NULL)
, m_nLLDataPos(0)
{

	// TODO: 在此处添加构造代码，
	// 将所有重要的初始化放置在 InitInstance 中
}

// 唯一的一个 CMy2BoxApp 对象

CMy2BoxApp theApp;

// CMy2BoxApp 初始化

BOOL CMy2BoxApp::InitInstance()
{
	if (FALSE == InitSelfPath())
	{
		AfxMessageBox(_T("自身路径获取失败"));
		return FALSE;
	}	

	if (FALSE == InitEnv())
	{
		AfxMessageBox(_T("环境块初始化失败"));
		return FALSE;
	}

	SRPC_INIT_BEGIN()
		//EXPORT_PROC("2Box.HookCompleted",&RpcServer::HookCompleted)
		EXPORT_PROC("2Box.NewProcess",&RpcServer::OnNewProcess)
		EXPORT_PROC("2Box.NewWnd",&RpcServer::OnNewWnd)
		EXPORT_PROC("2Box.EmbedWnd",&RpcServer::OnEmbedWnd)
		EXPORT_PROC("2Box.ActiveWnd",&RpcServer::OnActiveWnd)
		EXPORT_PROC("2Box.GetAllWnd",&RpcServer::OnGetAllWnd)
		//EXPORT_PROC("2Box.IsProcessInBox",&RpcServer::IsProcessInBox)
		//EXPORT_PROC("2Box.InitAllRemoteDlls",&RpcServer::InitAllRemoteDlls)
		//EXPORT_PROC("2Box.UnmapRemoteModule",&RpcServer::UnmapRemoteModule)
	SRPC_INIT_END()

	if (SRPC_INIT_FAIL)
	{
		AfxMessageBox(_T("rpc库初始化失败!程序可能无法正常运行"));
		return FALSE;
	}

	if (CheckInstance())
	{
		SRPC_Uninit();
		return FALSE;
	}

// 	if (FALSE == InitResource())
// 	{
// 		AfxMessageBox(_T("资源文件损坏"));
// 		return FALSE;
// 	}

	SRPC_SetAcptEventCb(NULL,&RpcServer::AcptEvent,NULL);

	m_rpcServer = SRPC_CreateServer(FD_LOCAL,"2Box_{BE6D2570-E5E5-40f0-9A53-CBDDE9C422BD}",0);
	if (NULL == m_rpcServer)
	{
		SRPC_Uninit();
		AfxMessageBox(_T("rpc库初始化失败!程序可能无法正常运行"));
		return FALSE;
	}	

	// 如果一个运行在 Windows XP 上的应用程序清单指定要
	// 使用 ComCtl32.dll 版本 6 或更高版本来启用可视化方式，
	//则需要 InitCommonControlsEx()。否则，将无法创建窗口。
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// 将它设置为包括所有要在应用程序中使用的
	// 公共控件类。
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinApp::InitInstance();

	// 标准初始化
	// 如果未使用这些功能并希望减小
	// 最终可执行文件的大小，则应移除下列
	// 不需要的特定初始化例程
	// 更改用于存储设置的注册表项
	// TODO: 应适当修改该字符串，
	// 例如修改为公司或组织名
	SetRegistryKey(_T("2Box"));
	LoadStdProfileSettings(4);

	// 若要创建主窗口，此代码将创建新的框架窗口
	// 对象，然后将其设置为应用程序的主窗口对象
	CMainFrame* pFrame = new CMainFrame;
	if (!pFrame)
		return FALSE;
	m_pMainWnd = pFrame;
	// 创建并加载框架及其资源
	pFrame->LoadFrame(IDR_MAINFRAME,
		WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL,
		NULL);

	// 唯一的一个窗口已初始化，因此显示它并对其进行更新
	pFrame->ShowWindow(SW_SHOW);
	pFrame->UpdateWindow();
	// 仅当具有后缀时才调用 DragAcceptFiles
	//  在 SDI 应用程序中，这应在 ProcessShellCommand 之后发生
	return TRUE;
}

CDocument* CMy2BoxApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
	// TODO: 在此添加专用代码和/或调用基类
	CFileFind finder;
	if(finder.FindFile(lpszFileName))
	{
		CString s(lpszFileName);
		StartLoadFileThread(s);
		return (CDocument*)1;
	}
	else
	{
		AfxMessageBox(_T("文件不存在！"));
		return NULL;
	}

	//return CWinApp::OpenDocumentFile(lpszFileName);
}

BOOL CMy2BoxApp::OnIdle(LONG lCount)
{
	// TODO: 在此添加专用代码和/或调用基类

	if (CWinApp::OnIdle(lCount))
	{
		return TRUE;
	}

	if (lCount == 2)
	{
		((CMainFrame*)GetMainWnd())->UpdateViewWhenIdle();
		return FALSE;
	}

	return TRUE;
}

// CMy2BoxApp 消息处理程序

void CMy2BoxApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

void CMy2BoxApp::OnFileOpen()
{
	// TODO: 在此添加命令处理程序代码
	CString strFile;  

	CFileDialog    dlgFile(TRUE);  

	if (IDOK == dlgFile.DoModal())  
	{  
		strFile = dlgFile.GetPathName(); 
		AddToRecentFileList(strFile);
		OpenDocumentFile(strFile);
	}  	
}

void CMy2BoxApp::PreDestroyMainWnd()
{
	if (m_rpcServer)
	{
		SRPC_DestroyServer(m_rpcServer);
	}

	SRPC_Uninit();
}

UINT CMy2BoxApp::GetSettingsInt( LPCTSTR lpszEntry, int nDefault )
{
	return GetProfileInt(_T("Settings"),lpszEntry,nDefault);
}

CString CMy2BoxApp::GetSettingsString( LPCTSTR lpszEntry, LPCTSTR lpszDefault /*= NULL*/ )
{
	return GetProfileString(_T("Settings"),lpszEntry,lpszDefault);
}

BOOL CMy2BoxApp::WriteSettingsInt( LPCTSTR lpszEntry, int nValue )
{
	return WriteProfileInt(_T("Settings"),lpszEntry,nValue);
}

BOOL CMy2BoxApp::WriteSettingsString( LPCTSTR lpszEntry, LPCTSTR lpszValue )
{
	return WriteProfileString(_T("Settings"),lpszEntry,lpszValue);
}

BOOL CMy2BoxApp::GetNewEnv(std::wstring& strOut) const
{
	_timeb timebuffer;
	_ftime_s(&timebuffer);
	unsigned long long llData = timebuffer.time * 1000 + timebuffer.millitm;
	wchar_t szLLData[21] = {0};	

	swprintf_s(szLLData,21,L"%020I64u",llData);
	strOut = m_strEnv;
	strOut.replace(m_nLLDataPos,20,szLLData,20);
	return TRUE;
}

BOOL CMy2BoxApp::InitEnv()
{
	LPWCH pEnv = GetEnvironmentStringsW();
	if (NULL == pEnv)
	{
		return FALSE;
	}	

	BOOL bProcRet = ProcUnknownEnvStringsW(pEnv,(const wchar_t*)m_strSelfPathW,0,0);
	FreeEnvironmentStringsW(pEnv);
	return bProcRet;
}

BOOL CMy2BoxApp::ProcUnknownEnvStringsW(const void* pszzEnv,
										const std::wstring& loaderPath, 
										unsigned long long llData, 
										unsigned long lEventNo)
{
	const wchar_t* toParse = (const wchar_t*)pszzEnv;
	if (((const char*)toParse)[1]) 
	{
		//大概率不是unicode字符;
		return FALSE;
	}

	size_t nSize = 0;
	while(1)
	{
		if (0 == toParse[nSize++] && 0 == toParse[nSize++])
		{
			break;
		}
	}
	m_strEnv.append(toParse,nSize-1);

	m_strEnv += L"BoxMonLibPath=";
	m_strEnv += loaderPath;
	m_strEnv += L'\0';

	wchar_t szName[21] = {0};
	swprintf_s(szName,21,L"%020I64u",llData);

	m_strEnv += L"BoxLLData=";
	m_nLLDataPos = m_strEnv.size();
	m_strEnv += szName;
	m_strEnv += L'\0';	

	m_strEnv += L'\0';
	return TRUE;
}

BOOL CMy2BoxApp::CheckInstance()
{
	if (FALSE == WaitNamedPipe(_T("\\\\.\\pipe\\2Box_{BE6D2570-E5E5-40f0-9A53-CBDDE9C422BD}"),1))
	{
		return FALSE;
	}

	void* client = SRPC_CreateClient(FD_LOCAL,"2Box_{BE6D2570-E5E5-40f0-9A53-CBDDE9C422BD}",0);
	if (client)
	{
		SRPC_Call(client,"2Box.ActiveWnd",NULL,0,NULL,NULL);
		SRPC_DestroyClient(client);
	}
	return TRUE;
}

BOOL CMy2BoxApp::StartLoadFileThread( CString& FileName )
{
	CString* pstrName = new CString(FileName);
	if (0 != SRPC_ThreadPoolWork(NULL,&NewFileThread,pstrName))
	{
		delete pstrName;
		AfxMessageBox(_T("程序启动失败"));
		return FALSE;
	}

	return TRUE;
}

BOOL CMy2BoxApp::InitSelfPath()
{
	wchar_t szPathW[MAX_PATH + 1] = {0};
	char szPathA[MAX_PATH + 1] = {0};

	if (0 == GetModuleFileNameW(NULL,szPathW,MAX_PATH)
		|| 0 == GetModuleFileNameA(NULL,szPathA,MAX_PATH))
	{
		return FALSE;
	}

	m_strSelfPathW = szPathW;
	m_strSelfPathW = m_strSelfPathW.Left(m_strSelfPathW.ReverseFind('\\')+1);

	m_strSelfPathA = szPathA;
	m_strSelfPathA = m_strSelfPathA.Left(m_strSelfPathA.ReverseFind('\\')+1);	
	return TRUE;
}

// BOOL CMy2BoxApp::InitResource()
// {
// 	wchar_t szPathW[MAX_PATH + 2] = {0};
// 	swprintf_s(szPathW,MAX_PATH,L"%s%s",(const wchar_t*)m_strSelfPathW,L"2BoxFileSystem");
// 
// 	SHFILEOPSTRUCTW Op = {0};
// 	Op.wFunc = FO_DELETE; 	
// 	Op.pFrom = szPathW;     
// 	Op.fFlags = FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR; 
// 
// 	SHFileOperationW(&Op);
// 
// 	CreateDirectoryW( szPathW,NULL);
// 
// // 	if (FALSE == CreateResource(MAKEINTRESOURCE(IDR_PE_HELPER),m_strSelfPathW + _T("2Box_x64_helper.dll"))
// // 		|| FALSE == CreateResource(MAKEINTRESOURCE(IDR_PE_MONITOR32),m_strSelfPathW + _T("2BoxMonitor32.dll"))
// // 		|| FALSE == CreateResource(MAKEINTRESOURCE(IDR_PE_MONITOR64),m_strSelfPathW + _T("2BoxMonitor64.dll")))
// // 	{
// // 		return FALSE;
// // 	}
// 
// 	return TRUE;
// }

// BOOL CMy2BoxApp::CreateResource(LPCTSTR lpResName,const CString& FilePath)
// {
// 	BOOL bRetVal = FALSE;
// 	HRSRC hRes = NULL;
// 	HGLOBAL hResourceLoaded = NULL;
// 	HANDLE hFile = INVALID_HANDLE_VALUE;
// 
// 	do 
// 	{
// 		hRes = FindResource(AfxGetInstanceHandle(),lpResName,_T("PE"));
// 		if (NULL == hRes)
// 		{
// 			break;
// 		}
// 
// 		hResourceLoaded = LoadResource(AfxGetInstanceHandle(), hRes); 
// 		if (NULL == hResourceLoaded)
// 		{
// 			break;
// 		}
// 
// 		// 得到资源文件大小 ;
// 		DWORD dwSize = SizeofResource(AfxGetInstanceHandle(), hRes); 
// 		// 锁定资源以得到它在内存中的地址 ;
// 		LPVOID sRawPE = (LPVOID)LockResource(hResourceLoaded);
// 
// 		hFile = CreateFile(FilePath,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
// 		if (INVALID_HANDLE_VALUE == hFile)
// 		{
// 			break;
// 		}
// 
// 		DWORD  dwWrited = 0;
// 		if (FALSE == WriteFile(hFile,sRawPE,dwSize,&dwWrited,NULL)
// 			|| dwWrited != dwSize)
// 		{
// 			break;
// 		}
// 
// 		bRetVal = TRUE;
// 
// 	} while (0);
// 
// 	if (hFile != INVALID_HANDLE_VALUE)
// 	{
// 		CloseHandle(hFile); 
// 	}
// 
// 	if (hResourceLoaded)
// 	{
// 		FreeResource(hResourceLoaded);
// 	}
// 	return bRetVal;
// }

