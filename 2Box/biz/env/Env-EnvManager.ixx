export module Env:EnvManager;

import std;
import :Envrironment;

namespace biz
{
	export class EnvManager
	{
	public:
		static EnvManager& instance()
		{
			static EnvManager sInstance;
			return sInstance;
		}

		void loadEnvFrom(std::uint32_t index, std::uint64_t flag, std::wstring_view flagName, std::wstring_view name);
		std::shared_ptr<Env> createEnv();
		std::shared_ptr<Env> findEnvByFlagNoExcept(std::uint64_t flag);
		std::shared_ptr<Env> findEnvByFlag(std::uint64_t flag);
		std::shared_ptr<Env> testFindFirstOrCreate();

	private:
		EnvManager() = default;
		
		void addEnv(const std::shared_ptr<Env>& env);

	private:
		std::atomic<std::uint32_t> m_currentIndex{0};
		std::mutex m_mutex;
		std::map<std::uint32_t, std::shared_ptr<Env>> m_orderedEnvs;
		std::unordered_map<std::uint64_t, std::shared_ptr<Env>> m_flagToEnv;
	};
}
