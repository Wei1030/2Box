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
		void renameEnv(const std::shared_ptr<Env>& env, std::wstring_view newName);

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
			Delete,
			// 删除容器后，编号更大的容器会自动前移，此事件用于通知界面刷新卡片编号映射
			Renumber
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

		// 删除某个编号的容器后，将编号更大的容器依次前移以保持编号连续（不改变环境显示名称）
		void renumberEnvsAfter(const std::vector<std::shared_ptr<Env>>& envsToRenumber, std::uint32_t deletedIndex);

	private:
		mutable std::shared_mutex m_mutex;
		std::unordered_map<std::uint64_t, std::shared_ptr<Env>> m_flagToEnv;
		// 当前已被占用的容器编号，创建新容器时优先复用最小空闲编号
		std::set<std::uint32_t> m_usedIndices;
		EnvChangeNotify m_envChangeNotify;
	};
}
