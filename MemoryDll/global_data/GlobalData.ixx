export module GlobalData;

import "sys_defs.h";
import std;

namespace global
{
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

	export class Data
	{
	public:
		static Data& get()
		{
			static Data instance;
			return instance;
		}

	public:
		void initialize(SystemVersionInfo versionInfo, std::uint64_t envFlag, unsigned long envIndex, std::wstring_view rootPath);

	public:
		SystemVersionInfo sysVersion() const { return m_sysVersion; }
		std::uint64_t envFlag() const { return m_envFlag; }
		std::uint32_t envIndex() const { return m_envIndex; }
		bool isNonLimitedAdmin() const { return m_bIsNonLimitedAdmin; }
		bool isCmd() const { return m_bIsCmd; }
		std::wstring_view envFlagName() const { return m_envFlagName; }
		std::string_view envFlagNameA() const { return m_envFlagNameA; }
		std::wstring_view rootPath() const { return m_rootPath; }
		std::string_view dllFullPath() const { return m_dllFullPath; }
		HKEY appKey() const { return m_appKey; }
		std::uint32_t inputSyncMsgId() const { return m_inputSyncMsgId; }

		bool isInKnownFolderPath(std::wstring_view path) const;
		std::optional<std::wstring> getRedirectPath(std::wstring_view knownFolderPath) const;

	private:
		Data() = default;

		void initializePrivilegesAbout();
		void initializeRegistry();
		void initializeSelfPath();
		void initializeDllFullPath();
		void initializeKnownFolderPath();
		void initializeMisc();

	private:
		SystemVersionInfo m_sysVersion;
		std::uint64_t m_envFlag{0};
		std::uint32_t m_envIndex{0};
		bool m_bIsNonLimitedAdmin{false};
		bool m_bIsCmd{false};
		std::wstring m_envFlagName;
		std::string m_envFlagNameA;
		std::string m_dllFullPath;
		std::wstring m_rootPath;
		std::wstring m_selfFullPath;
		std::wstring m_selfFileName;
		RegKey m_appKey;
		std::vector<std::wstring> m_knownFolders;
		std::uint32_t m_inputSyncMsgId{0};
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

	export bool ensure_dir_exists(std::wstring_view fullName, bool bIsDir);
}
