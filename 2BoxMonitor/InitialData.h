#pragma once

#include "RPCData.h"

class CInitialData
{
public:
	CInitialData(void);
	~CInitialData(void);
	
	BOOL Init();

	unsigned long long GetllData(){return m_llData;}
	const char* GetNewNameA(){return m_szNewNameA;}
	const wchar_t* GetNewNameW(){return m_szNewNameW;}

	const char* GetDllPath() const {return m_strDllPath.c_str();}

	BOOL ProcUnknownEnvStringsW(const void* pszzEnv,std::wstring& strOut) const;
	BOOL ProcUnknownEnvStringsA(const void* pszzEnv,std::string& strOut) const;

private:
	unsigned long long m_llData;
	char m_szNewNameA[32];
	wchar_t m_szNewNameW[32];

	wchar_t m_szSelfPathW[MAX_PATH+1];
	char	m_szSelfPathA[MAX_PATH+1];	

	std::string m_strDllPath;
};

extern CInitialData* g_pData;