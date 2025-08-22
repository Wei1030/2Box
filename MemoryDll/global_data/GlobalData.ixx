// module;
// #define _CRT_SECURE_NO_WARNINGS
// #include <cstdio>
export module GlobalData;

import "sys_defs.h";
import std;

namespace global
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

	export class RegKey
	{
	public:
		RegKey() = default;

		explicit RegKey(auto creator)
		{
			m_key = creator();
		}

		~RegKey()
		{
			if (m_key)
			{
				RegCloseKey(m_key);
			}
		}

		RegKey(const RegKey&) = delete;
		RegKey& operator=(const RegKey&) = delete;

		RegKey(RegKey&& that) noexcept : m_key(std::exchange(that.m_key, nullptr))
		{
		}

		RegKey& operator=(RegKey&& that) noexcept
		{
			std::swap(m_key, that.m_key);
			return *this;
		}

		operator HKEY() const { return m_key; }

	private:
		HKEY m_key{nullptr};
	};

	// std::wstring_view get_current_user_sid()
	// {
	// 	HANDLE hToken;
	// 	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
	// 	{
	// 		throw std::runtime_error{std::format("OpenProcessToken failed, error:{}\n", GetLastError())};
	// 	}
	//
	// 	DWORD tuSize = 0;
	// 	GetTokenInformation(hToken, TokenUser, nullptr, 0, &tuSize);
	// 	if (!tuSize)
	// 	{
	// 		throw std::runtime_error{std::format("GetTokenInformation failed, error:{}\n", GetLastError())};
	// 	}
	// 	std::vector<std::byte> buffer(tuSize);
	// 	if (!GetTokenInformation(hToken, TokenUser, buffer.data(), static_cast<DWORD>(buffer.size()), &tuSize))
	// 	{
	// 		throw std::runtime_error{std::format("GetTokenInformation failed, error:{}\n", GetLastError())};
	// 	}
	// 	const TOKEN_USER* pTu = reinterpret_cast<TOKEN_USER*>(buffer.data());
	// 	wchar_t* pStrSid = nullptr;
	// 	if (!ConvertSidToStringSidW(pTu->User.Sid, &pStrSid))
	// 	{
	// 		throw std::runtime_error{std::format("ConvertSidToStringSidW failed, error:{}\n", GetLastError())};
	// 	}
	// 	return {pStrSid};
	// }

	export class Data
	{
	public:
		static Data& get()
		{
			static Data instance;
			return instance;
		}

	public:
		void initialize(std::uint64_t envFlag, std::wstring_view envPath)
		{
			m_envFlag = envFlag;
			m_envFlagName = std::format(L"{:016X}", envFlag);
			m_envFlagNameA = std::format("{:016X}", envFlag);
			m_envPath = envPath;
			
			initializeRegistry();
			initializeDllFullPath();
			
			// std::wcout.imbue(std::locale(""));
			// InitConsole();
		}

	public:
		std::uint64_t envFlag() const
		{
			return m_envFlag;
		}

		std::wstring_view envFlagName() const
		{
			return m_envFlagName;
		}

		std::string_view envFlagNameA() const
		{
			return m_envFlagNameA;
		}

		std::wstring_view envPath() const
		{
			return m_envPath;
		}

		std::string_view dllFullPath() const
		{
			return m_dllFullPath;
		}

		HKEY appKey() const
		{
			return m_appKey;
		}

	private:
		Data() = default;

		void initializeRegistry()
		{
			m_appKey = RegKey{
				[&]()-> HKEY
				{
					namespace fs = std::filesystem;
					const fs::path envFile{fs::weakly_canonical(fs::path{m_envPath} / fs::path{m_envFlagName})};
					HKEY hKey;
					if (RegLoadAppKeyW(envFile.native().c_str(), &hKey, KEY_ALL_ACCESS, 0, 0) != ERROR_SUCCESS)
					{
						throw std::runtime_error("Failed to load app key");
					}
					return hKey;
				}
			};
		}

		void initializeDllFullPath()
		{
			namespace fs = std::filesystem;
			if constexpr (CURRENT_ARCH_BIT == ArchBit::Bit64)
			{
				m_dllFullPath = fs::path{fs::weakly_canonical(fs::path{m_envPath} / fs::path{std::format(L"{}_64.bin", m_envFlagName)})}.string();
			}
			else
			{
				m_dllFullPath = fs::path{fs::weakly_canonical(fs::path{m_envPath} / fs::path{std::format(L"{}_32.bin", m_envFlagName)})}.string();
			}
		}

	private:
		std::uint64_t m_envFlag{0};
		std::wstring m_envFlagName;
		std::string m_envFlagNameA;
		std::string m_dllFullPath;
		std::wstring m_envPath;
		RegKey m_appKey;
	};

	export bool is_app_key_name(std::wstring_view fullName)
	{
		static constexpr std::wstring_view prefixToFind(LR"(\REGISTRY\A\{)");
		return fullName.starts_with(prefixToFind);
	}

	export std::wstring_view remove_leading_backslashes_sv(std::wstring_view sv)
	{
		const auto it = std::find_if_not(sv.begin(), sv.end(), [](wchar_t c) { return c == L'\\'; });
		if (it == sv.end())
		{
			return sv;
		}
		return sv.substr(std::distance(sv.begin(), it));
	}
}
