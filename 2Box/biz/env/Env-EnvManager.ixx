export module Env:EnvManager;

import std;
import :Envrironment;

namespace biz
{
	export class EnvManager
	{
	public:
		EnvManager();
		void loadEnvFrom(std::uint32_t index, std::uint64_t flag, std::wstring_view flagName, std::wstring_view name);
		std::shared_ptr<Env> createEnv();
		std::shared_ptr<Env> findEnvByFlagNoExcept(std::uint64_t flag) const;
		std::shared_ptr<Env> findEnvByFlag(std::uint64_t flag) const;
		std::size_t getEnvCount() const;

		void deleteEnv(std::shared_ptr<Env> env);

		bool containsProcessIdExclude(std::uint32_t pid, std::uint64_t excludeEnvFlag) const;
		std::vector<DWORD> getAllProcessIdsExclude(std::uint64_t excludeEnvFlag) const;

		bool containsToplevelWindowExclude(void* hWnd, std::uint64_t excludeEnvFlag) const;
		std::vector<void*> getAllToplevelWindows() const;
		std::vector<void*> getAllToplevelWindowsExclude(std::uint64_t excludeEnvFlag) const;

	public:
		std::vector<std::shared_ptr<Env>> getAllEnv() const;

		enum class EChangeType:std::uint8_t
		{
			Create,
			Delete
		};

		using EnvChangeNotify = std::function<void(EChangeType, const std::shared_ptr<Env>&)>;
		void setEnvChangeNotify(EnvChangeNotify envChangeNotify);

	private:
		struct EnvFlagInfo
		{
			std::uint64_t flag;
			std::wstring flagName;
		};

		EnvFlagInfo ensureCreateNewEnvFlag(std::uint32_t index) const;

		void addEnv(const std::shared_ptr<Env>& env);
		void removeEnv(std::uint64_t flag);

	private:
		std::atomic<std::uint32_t> m_currentIndex{0};
		mutable std::shared_mutex m_mutex;
		std::unordered_map<std::uint64_t, std::shared_ptr<Env>> m_flagToEnv;
		EnvChangeNotify m_envChangeNotify;
	};
}
