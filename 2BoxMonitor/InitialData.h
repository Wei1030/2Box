#pragma once

class CInitialData
{
public:
	CInitialData(void);
	~CInitialData(void);
	
	BOOL Init();

	unsigned long long GetllData(){return m_llData;}
	const char* GetNewNameA(){return m_szNewNameA;}
	const wchar_t* GetNewNameW(){return m_szNewNameW;}

	const char* GetDllPathA() const {return m_strDllPath.c_str();}
	const wchar_t* GetSelfPathW() const {return m_szSelfPathW;}

	const wchar_t* GetSysPathW() const {return m_szSysPathW;}
	const wchar_t* GetWinPathW() const {return m_szWinPathW;}

	BOOL ProcUnknownEnvStringsW(const void* pszzEnv,std::wstring& strOut) const;
	BOOL ProcUnknownEnvStringsA(const void* pszzEnv,std::string& strOut) const;

	void AddFilesToMgr(const std::wstring& sysFile);
	BOOL IsAllowedAccess(std::wstring& sysFile);

private:
	unsigned long long m_llData;
	char m_szNewNameA[32];
	wchar_t m_szNewNameW[32];

	wchar_t m_szSelfPathW[MAX_PATH+1];
	char	m_szSelfPathA[MAX_PATH+1];

	wchar_t m_szSysPathW[MAX_PATH+1];
	wchar_t m_szWinPathW[MAX_PATH+1];

	std::string m_strDllPath;

	std::set<std::wstring> m_mapFiles;
};

extern CInitialData* g_pData;