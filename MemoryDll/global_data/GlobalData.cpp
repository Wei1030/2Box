// ReSharper disable CppUseRangeAlgorithm
module;
// #define _CRT_SECURE_NO_WARNINGS
// #include <cstdio>
module GlobalData;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

namespace
{
	// void InitConsole()
	// {
	// 	if (!AllocConsole())
	// 	{
	// 		return;
	// 	}
	//
	// 	freopen("CONOUT$", "w", stdout);
	// }

	BOOL get_process_elevation(TOKEN_ELEVATION_TYPE* pElevationType, BOOL* pIsAdmin)
	{
		HANDLE hToken{nullptr};
		DWORD dwSize;

		// Get current process token
		if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
		{
			return FALSE;
		}

		BOOL bResult = FALSE;
		// Retrieve elevation type information 
		if (GetTokenInformation(hToken, TokenElevationType,
		                        pElevationType, sizeof(TOKEN_ELEVATION_TYPE), &dwSize))
		{
			// Create the SID corresponding to the Administrators group
			byte adminSid[SECURITY_MAX_SID_SIZE]{};
			dwSize = sizeof(adminSid);
			CreateWellKnownSid(WinBuiltinAdministratorsSid, nullptr, &adminSid, &dwSize);

			if (*pElevationType == TokenElevationTypeLimited)
			{
				// Get handle to linked token (will have one if we are lua)
				HANDLE hUnfilteredToken{nullptr};
				if (GetTokenInformation(hToken, TokenLinkedToken,
				                        &hUnfilteredToken, sizeof(HANDLE), &dwSize))
				{
					// Check if this original token contains admin SID
					if (CheckTokenMembership(hUnfilteredToken, &adminSid, pIsAdmin))
					{
						bResult = TRUE;
					}
					CloseHandle(hUnfilteredToken);
				}
			}
			else
			{
				*pIsAdmin = IsUserAnAdmin();
				bResult = TRUE;
			}
		}
		CloseHandle(hToken);
		return bResult;
	}
}

namespace global
{
	void Data::initialize(SystemVersionInfo versionInfo, std::uint64_t envFlag, unsigned long envIndex, std::wstring_view rootPath)
	{
		m_sysVersion = versionInfo;
		m_envFlag = envFlag;
		m_envIndex = envIndex;
		m_envFlagName = std::format(L"{:016X}", envFlag);
		m_envFlagNameA = std::format("{:016X}", envFlag);
		m_rootPath = rootPath;

		initializePrivilegesAbout();
		initializeRegistry();
		initializeSelfPath();
		initializeDllFullPath();
		initializeKnownFolderPath();
		initializeMisc();

		// std::wcout.imbue(std::locale(""));
		// InitConsole();
	}

	static constexpr std::wstring_view PREFIX_TO_CHECK(LR"(\??\)");

	bool Data::isInKnownFolderPath(std::wstring_view path) const
	{
		if (m_knownFolders.empty())
		{
			return false;
		}

		if (!path.starts_with(PREFIX_TO_CHECK))
		{
			return false;
		}

		std::wstring_view pathToCheck = path.substr(PREFIX_TO_CHECK.length());
		std::wstring lowerPath(pathToCheck);
		std::transform(lowerPath.begin(), lowerPath.end(), lowerPath.begin(), std::towlower);

		if (lowerPath.contains(L"microsoft")
			|| lowerPath.contains(L"nvidia")
			|| lowerPath.contains(L"amd")
			|| lowerPath.contains(LR"(\2box\env\)"))
		{
			return false;
		}
		//auto toLowerIterNow = lowerPath.begin();
		for (const std::wstring& knownFolder : m_knownFolders)
		{
			if (knownFolder.length() > lowerPath.length())
			{
				continue;
			}
			// if (const size_t alreadyToLowerCount = toLowerIterNow - lowerPath.begin();
			// 	alreadyToLowerCount < knownFolder.length())
			// {
			// 	const size_t needToLowerCount = knownFolder.length() - alreadyToLowerCount;
			// 	const auto last = toLowerIterNow + needToLowerCount;
			// 	std::transform(toLowerIterNow, last, toLowerIterNow, std::towlower);
			// 	toLowerIterNow = last;
			// }
			if (!lowerPath.starts_with(knownFolder))
			{
				continue;
			}
			return true;
		}
		return false;
	}

	std::optional<std::wstring> Data::getRedirectPath(std::wstring_view knownFolderPath) const
	{
		static constexpr std::wstring_view driverMarker(LR"(:\)");

		try
		{
			namespace fs = std::filesystem;
			if (const size_t driverPos = knownFolderPath.find(driverMarker); driverPos != std::wstring_view::npos)
			{
				const fs::path indexPath{std::format(L"{}", m_envIndex)};
				const fs::path relativePath{knownFolderPath.substr(driverPos + driverMarker.length())};
				const fs::path redirectPath{fs::weakly_canonical(fs::path{m_rootPath} / fs::path{L"Env"} / indexPath / relativePath)};
				return std::format(L"{}{}", PREFIX_TO_CHECK, redirectPath.native());
			}
		}
		catch (...)
		{
		}
		return std::nullopt;
	}

	void Data::initializePrivilegesAbout()
	{
		TOKEN_ELEVATION_TYPE elevationType = TokenElevationTypeDefault;
		BOOL bIsAdmin = FALSE;
		if (get_process_elevation(&elevationType, &bIsAdmin))
		{
			if (elevationType != TokenElevationTypeLimited)
			{
				m_bIsNonLimitedAdmin = bIsAdmin ? true : false;
			}
		}
	}

	void Data::initializeRegistry()
	{
		m_appKey = RegKey{
			[&]()-> HKEY
			{
				namespace fs = std::filesystem;
				const fs::path envFile{fs::weakly_canonical(fs::path{m_rootPath} / fs::path{L"Env"} / fs::path{std::format(L"{}", m_envIndex)} / fs::path{m_envFlagName})};
				HKEY hKey;
				if (RegLoadAppKeyW(envFile.native().c_str(), &hKey, KEY_ALL_ACCESS, 0, 0) != ERROR_SUCCESS)
				{
					throw std::runtime_error("Failed to load app key");
				}
				return hKey;
			}
		};
	}

	void Data::initializeSelfPath()
	{
		constexpr DWORD pathLength = std::numeric_limits<short>::max();
		m_selfFullPath.resize(pathLength);
		DWORD resultSize = GetModuleFileNameW(nullptr, m_selfFullPath.data(), pathLength);
		m_selfFullPath.resize(resultSize);
		m_selfFullPath = std::wstring(m_selfFullPath);
		m_selfFileName = std::filesystem::path{m_selfFullPath}.filename().native();
		m_bIsCmd = _wcsicmp(m_selfFileName.c_str(), L"cmd.exe") == 0;
	}

	void Data::initializeDllFullPath()
	{
		namespace fs = std::filesystem;
		if constexpr (CURRENT_ARCH_BIT == ArchBit::Bit64)
		{
			m_dllFullPath = fs::path{fs::weakly_canonical(fs::path{m_rootPath} / fs::path{L"bin"} / fs::path{std::format(L"{}_64.bin", m_envFlagName)})}.string();
		}
		else
		{
			m_dllFullPath = fs::path{fs::weakly_canonical(fs::path{m_rootPath} / fs::path{L"bin"} / fs::path{std::format(L"{}_32.bin", m_envFlagName)})}.string();
		}
	}

	void Data::initializeKnownFolderPath()
	{
		static const std::array rfidArray = {FOLDERID_LocalAppData, FOLDERID_LocalAppDataLow, FOLDERID_RoamingAppData, FOLDERID_SavedGames, FOLDERID_ProgramData};

		for (size_t i = 0; i < rfidArray.size(); ++i)
		{
			const KNOWNFOLDERID& rfid = rfidArray[i];
			wchar_t* out;
			if (S_OK == SHGetKnownFolderPath(rfid, 0, nullptr, &out))
			{
				std::wstring_view sv{out};
				std::transform(sv.begin(), sv.end(), out, std::towlower);
				if (std::optional<std::wstring> redirectPath = getRedirectPath(sv))
				{
					if (ensure_dir_exists(redirectPath.value(), true))
					{
						m_knownFolders.push_back(std::format(L"{}\\", sv));
					}
				}
				CoTaskMemFree(out);
			}
		}
	}

	void Data::initializeMisc()
	{
		m_inputSyncMsgId = RegisterWindowMessageW(L"2Box_WM_INPUT_SYNC");
		if (!m_inputSyncMsgId)
		{
			m_inputSyncMsgId = 9527;
		}
	}

	bool ensure_dir_exists(std::wstring_view fullName, bool bIsDir)
	{
		try
		{
			namespace fs = std::filesystem;
			const fs::path path{fullName};
			if (bIsDir)
			{
				fs::create_directories(path);
			}
			else
			{
				fs::create_directories(path.parent_path());
			}
			return true;
		}
		catch (...)
		{
		}
		return false;
	}
}
