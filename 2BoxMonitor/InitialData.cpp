#include "stdafx.h"
#include "InitialData.h"

#include <algorithm>

CInitialData* g_pData = new CInitialData;

// void InitConsole()
// {	
// 	if(!AllocConsole())
// 	{
// 		return;
// 	}
// 
// 	freopen("CONOUT$", "w", stdout);
// }

CInitialData::CInitialData(void)
: m_llData(0)
{
	memset(m_szNewNameA,0,sizeof(m_szNewNameA));
	memset(m_szNewNameW,0,sizeof(m_szNewNameW));
	memset(m_szSelfPathW,0,sizeof(m_szSelfPathW));
	memset(m_szSelfPathA,0,sizeof(m_szSelfPathA));
	memset(m_szSysPathW,0,sizeof(m_szSysPathW));

	//memset(&m_procData,0,sizeof(NEW_PROC_DATA));
}

CInitialData::~CInitialData(void)
{
}

BOOL CInitialData::Init()
{
	BOOL bRetVal = FALSE;
	do 
	{
		if (0 == GetSystemDirectoryW(m_szSysPathW,MAX_PATH))
		{
			break;
		}

		wchar_t szNewNameW[32] = {0};
		DWORD dwSize = GetEnvironmentVariableW(L"BoxLLData",szNewNameW,32);
		if (0 == dwSize || dwSize > 32)
		{
			break;
		}

		m_llData = _wtoi64(szNewNameW);
		if (0 == m_llData)
		{
			break;
		}

		sprintf_s(m_szNewNameA,32,"%I64u",m_llData);
		_strrev(m_szNewNameA);

		swprintf_s(m_szNewNameW,32,L"%I64u",m_llData);
		_wcsrev(m_szNewNameW);

		dwSize = GetEnvironmentVariableW(L"BoxMonLibPath",m_szSelfPathW,MAX_PATH+1);
		if (0 == dwSize || dwSize > MAX_PATH+1)
		{
			break;
		}

		dwSize = GetEnvironmentVariableA("BoxMonLibPath",m_szSelfPathA,MAX_PATH+1);
		if (0 == dwSize || dwSize > MAX_PATH+1)
		{
			break;
		}

		m_strDllPath = m_szSelfPathA;
#ifdef _WIN64
		m_strDllPath += "2BoxMonitor64.dll";
#else
		m_strDllPath += "2BoxMonitor32.dll";
#endif		
			
		//InitConsole();

		bRetVal = TRUE;

	} while (0);

	if (bRetVal)
	{
		SetEnvironmentVariableW(L"BoxLLData",NULL);
		SetEnvironmentVariableW(L"BoxMonLibPath",NULL);
	}

	return bRetVal;	
}

BOOL CInitialData::ProcUnknownEnvStringsW(const void* pszzEnv,std::wstring& strOut) const
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

		if (nSize >= 32767)
		{
			return FALSE;
		}
	}

	strOut.append(toParse,nSize-1);	

	//////////////////////////////////////////////////////////////////////////
	//MonLibPath
	size_t nMonLibPathPos = strOut.find(L"BoxMonLibPath=");
	if (nMonLibPathPos != std::wstring::npos)
	{
		nMonLibPathPos += 14;
		std::wstring strMonLibPath = (const wchar_t*)pszzEnv + nMonLibPathPos;
		strOut.replace(nMonLibPathPos,strMonLibPath.size(),m_szSelfPathW);
	}
	else
	{
		strOut += L"BoxMonLibPath=";
		strOut += m_szSelfPathW;
		strOut += L'\0';
	}

	//////////////////////////////////////////////////////////////////////////
	//ll data	
	wchar_t szName[21] = {0};
	swprintf_s(szName,21,L"%020I64u",m_llData);	
	size_t nLLDataPos = strOut.find(L"BoxLLData=");
	if (nLLDataPos != std::wstring::npos)
	{
		nLLDataPos += 10;
		std::wstring strLLData = (const wchar_t*)pszzEnv + nLLDataPos;
		strOut.replace(nLLDataPos,strLLData.size(),szName);
	}
	else
	{
		strOut += L"BoxLLData=";
		strOut += szName;
		strOut += L'\0';
	}	

	strOut += L'\0';
	return TRUE;
}

BOOL CInitialData::ProcUnknownEnvStringsA(const void* pszzEnv,std::string& strOut) const
{
	const char* toParse = (const char*)pszzEnv;
	if (0 == toParse[1]) 
	{
		//大概率不是ANSI字符;
		return FALSE;
	}

	size_t nSize = 0;
	while(1)
	{
		if (0 == toParse[nSize++] && 0 == toParse[nSize++])
		{
			break;
		}

		if (nSize >= 32767)
		{
			return FALSE;
		}
	}

	strOut.append(toParse,nSize-1);	

	//////////////////////////////////////////////////////////////////////////
	//MonLibPath
	size_t nMonLibPathPos = strOut.find("BoxMonLibPath=");
	if (nMonLibPathPos != std::string::npos)
	{
		nMonLibPathPos += 14;
		std::string strMonLibPath = (const char*)pszzEnv + nMonLibPathPos;
		strOut.replace(nMonLibPathPos,strMonLibPath.size(),m_szSelfPathA);
	}
	else
	{
		strOut += "BoxMonLibPath=";
		strOut += m_szSelfPathA;
		strOut += '\0';
	}

	//////////////////////////////////////////////////////////////////////////
	//ll data
	char szName[21] = {0};
	sprintf_s(szName,21,"%020I64u",m_llData);

	size_t nLLDataPos = strOut.find("BoxLLData=");
	if (nLLDataPos != std::string::npos)
	{
		nLLDataPos += 10;
		std::string strLLData = (const char*)pszzEnv + nLLDataPos;
		strOut.replace(nLLDataPos,strLLData.size(),szName);
	}
	else
	{
		strOut += "BoxLLData=";
		strOut += szName;
		strOut += '\0';
	}

	strOut += '\0';
	return TRUE;
}

void CInitialData::AddFilesToMgr(const std::wstring& sysFile,const std::wstring& fakeFile)
{
	std::wstring lowerSysFile = sysFile;
	std::wstring lowerFakeFile = fakeFile;

	std::transform(lowerSysFile.begin(), lowerSysFile.end(), lowerSysFile.begin(), tolower);
	std::transform(lowerFakeFile.begin(), lowerFakeFile.end(), lowerFakeFile.begin(), tolower);

	m_mapFiles.insert(std::make_pair(lowerSysFile,lowerFakeFile));
}

BOOL CInitialData::TryToChangeFileName(std::wstring& sysFile)
{
	std::map<std::wstring,std::wstring>::const_iterator it = m_mapFiles.find(sysFile);
	if (it == m_mapFiles.end())
	{
		return FALSE;
	}
	
	sysFile = it->second;
	return TRUE;
}

