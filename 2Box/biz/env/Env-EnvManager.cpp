module Env;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import std;
import MainApp;
import :Reg;

namespace
{
	std::uint64_t get_random_number()
	{
		thread_local std::mt19937_64 rng{std::random_device{}()};
		std::uniform_int_distribution<std::uint64_t> dis;
		return dis(rng);
	}

	struct EnvFileInfo
	{
		std::filesystem::path path;
		std::uint64_t flag;
		std::wstring flagName;
	};

	EnvFileInfo ensure_create_new_env_file()
	{
		namespace fs = std::filesystem;
		EnvFileInfo result;
		while (true)
		{
			const std::uint64_t flag = get_random_number();
			if (biz::EnvManager::instance().findEnvByFlagNoExcept(flag))
			{
				continue;
			}
			const std::wstring flagName = std::format(L"{:016X}", flag);
			const fs::path envPath{fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"} / fs::path{flagName})};
			if (fs::exists(envPath))
			{
				continue;
			}
			// create env file (app key)
			const biz::RegKey appKey{
				[&]()-> HKEY
				{
					HKEY hAppKey;
					if (LSTATUS status = RegLoadAppKeyW(envPath.native().c_str(), &hAppKey, KEY_ALL_ACCESS, 0, 0);
						status != ERROR_SUCCESS)
					{
						throw std::runtime_error(std::format("ensure_env_file_prepared failed, RegLoadAppKeyW result status:{}", status));
					}
					return hAppKey;
				}
			};
			result.flag = flag;
			result.flagName = flagName;
			result.path = envPath;
			break;
		}

		if (!fs::exists(result.path))
		{
			throw std::runtime_error{"unknown error in ensure_create_new_env_file!"};
		}
		return result;
	}
}

namespace biz
{
	void EnvManager::loadEnvFrom(std::uint32_t index, std::uint64_t flag, std::wstring_view flagName, std::wstring_view name)
	{
		if (index >= m_currentIndex.load(std::memory_order_relaxed))
		{
			m_currentIndex.store(index + 1, std::memory_order_relaxed);
		}
		namespace fs = std::filesystem;
		const fs::path envPath{fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"} / fs::path{flagName})};
		addEnv(std::make_shared<Env>(index, flag, flagName, name, envPath.native()));
	}

	std::shared_ptr<Env> EnvManager::createEnv()
	{
		std::shared_ptr<Env> envResult;
		auto [path, flag, flagName] = ensure_create_new_env_file();
		try
		{
			const std::uint32_t index = m_currentIndex.fetch_add(1, std::memory_order_relaxed);
			const std::wstring name = std::format(L"环境{}", index + 1);
			envResult = std::make_shared<Env>(index, flag, flagName, name, path.native());
			add_env_to_reg(flagName, envResult.get());
			addEnv(envResult);
		}
		catch (...)
		{
			namespace fs = std::filesystem;
			if (fs::exists(path))
			{
				fs::remove(path);
			}
			throw;
		}

		return envResult;
	}

	std::shared_ptr<Env> EnvManager::findEnvByFlagNoExcept(std::uint64_t flag)
	{
		std::lock_guard lock(m_mutex);
		const auto it = m_flagToEnv.find(flag);
		if (it == m_flagToEnv.end())
		{
			return nullptr;
		}
		return it->second;
	}

	std::shared_ptr<Env> EnvManager::findEnvByFlag(std::uint64_t flag)
	{
		std::shared_ptr<Env> result = findEnvByFlagNoExcept(flag);
		if (!result)
		{
			throw std::runtime_error(std::format("Failed to find env, flagName:{:016X}", flag));
		}
		return result;
	}

	std::shared_ptr<Env> EnvManager::testFindFirstOrCreate()
	{
		{
			std::lock_guard lock(m_mutex);
			if (m_orderedEnvs.size())
			{
				return m_orderedEnvs.begin()->second;
			}
		}
		return createEnv();
	}

	void EnvManager::addEnv(const std::shared_ptr<Env>& env)
	{
		std::lock_guard lock(m_mutex);
		const auto [it, success] = m_orderedEnvs.insert(std::make_pair(env->getIndex(), env));
		if (success)
		{
			try
			{
				if (!m_flagToEnv.insert(std::make_pair(env->getFlag(), env)).second)
				{
					throw std::runtime_error("add env failed! env flag error!");
				}
			}
			catch (...)
			{
				m_orderedEnvs.erase(it);
				throw;
			}
		}
		else
		{
			throw std::runtime_error("add env failed! env index error!");
		}
	}
}
