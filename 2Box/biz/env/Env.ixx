export module Env;

import "sys_defs.h";
import std;

namespace biz
{
	export class Env
	{
	public:
		void setIndex(DWORD index)
		{
			m_index = index;
		}

		void setFlag(std::uint64_t flag)
		{
			m_flags = flag;
		}

		DWORD getIndex() const { return m_index; }
		std::uint64_t getFlag() const { return m_flags; }

	private:
		DWORD m_index{0};
		std::uint64_t m_flags{0};
	};

	export class EnvManager
	{
	public:
		static EnvManager& instance()
		{
			static EnvManager sInstance;
			return sInstance;
		}

	public:
		bool contains(const std::wstring& name) const
		{
			return m_envMap.contains(name);
		}

		Env* getEnvByName(const std::wstring& name) const
		{
			const auto it = m_envMap.find(name);
			if (it == m_envMap.end())
			{
				return nullptr;
			}
			return it->second.get();
		}

	private:
		EnvManager();

	private:
		std::map<std::uint32_t, Env*> m_envs;
		std::unordered_map<std::wstring, std::unique_ptr<Env>> m_envMap;
	};

	export class Reg
	{
	public:
		static Reg& instance()
		{
			static Reg sInstance;
			return sInstance;
		}

	public:
		void initialize();
		void addEnv(const std::wstring& name, const Env* env) const;

	private:
		Reg() = default;

	private:
		HKEY m_appKey;
	};
}
