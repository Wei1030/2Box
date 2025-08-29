module Env;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import std;
import MainApp;
import :Reg;
import EssentialData;

namespace
{
	std::uint64_t get_random_number()
	{
		thread_local std::mt19937_64 rng{std::random_device{}()};
		std::uniform_int_distribution<std::uint64_t> dis;
		return dis(rng);
	}

	void ensure_dll_in_device(std::wstring_view flagName)
	{
		namespace fs = std::filesystem;
		if (const fs::path path32{biz::get_dll_full_path<ArchBit::Bit32>(flagName)}; !fs::exists(path32))
		{
			const fs::path tempPath{fs::weakly_canonical(biz::get_bin_path() / fs::path{std::format(L"{}_temp32.bin", flagName)})};
			const auto [address, size] = biz::get_dll_resource_inst<ArchBit::Bit32>();
			std::ofstream tempFile{tempPath, std::ios::binary | std::ios::trunc};
			tempFile.write(address, size);
			tempFile.close();
			fs::rename(tempPath, path32);
		}

		if (const fs::path path64{biz::get_dll_full_path<ArchBit::Bit64>(flagName)}; !fs::exists(path64))
		{
			const fs::path tempPath{fs::weakly_canonical(biz::get_bin_path() / fs::path{std::format(L"{}_temp64.bin", flagName)})};
			const auto [address, size] = biz::get_dll_resource_inst<ArchBit::Bit64>();
			std::ofstream tempFile{tempPath, std::ios::binary | std::ios::trunc};
			tempFile.write(address, size);
			tempFile.close();
			fs::rename(tempPath, path64);
		}
	}

	struct EnvFlagInfo
	{
		std::uint64_t flag;
		std::wstring flagName;
	};

	EnvFlagInfo ensure_create_new_env(std::uint32_t index)
	{
		namespace fs = std::filesystem;
		const fs::path envPath{fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"} / fs::path{std::format(L"{}", index)})};
		fs::create_directories(envPath);

		EnvFlagInfo result;
		while (true)
		{
			const std::uint64_t flag = get_random_number();
			if (biz::EnvManager::instance().findEnvByFlagNoExcept(flag))
			{
				continue;
			}
			const std::wstring flagName = std::format(L"{:016X}", flag);
			const fs::path envFile{fs::weakly_canonical(envPath / fs::path{flagName})};
			if (fs::exists(envFile))
			{
				continue;
			}

			ensure_dll_in_device(flagName);
			result.flag = flag;
			result.flagName = flagName;
			break;
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
		const fs::path envPath{fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"} / fs::path{std::format(L"{}", index)})};
		fs::create_directories(envPath);
		ensure_dll_in_device(flagName);
		addEnv(std::make_shared<Env>(index, flag, flagName, name, get_detours_injection_dll_name(flagName)));
	}

	std::shared_ptr<Env> EnvManager::createEnv()
	{
		std::shared_ptr<Env> envResult;
		const std::uint32_t index = m_currentIndex.fetch_add(1, std::memory_order_relaxed);
		auto [flag, flagName] = ensure_create_new_env(index);
		const std::wstring name = std::format(L"环境{}", index + 1);
		envResult = std::make_shared<Env>(index, flag, flagName, name, get_detours_injection_dll_name(flagName));
		add_env_to_reg(flagName, envResult.get());
		addEnv(envResult);
		return envResult;
	}

	std::shared_ptr<Env> EnvManager::findEnvByFlagNoExcept(std::uint64_t flag) const
	{
		std::shared_lock lock(m_mutex);
		const auto it = m_flagToEnv.find(flag);
		if (it == m_flagToEnv.end())
		{
			return nullptr;
		}
		return it->second;
	}

	std::shared_ptr<Env> EnvManager::findEnvByFlag(std::uint64_t flag) const
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
			std::shared_lock lock(m_mutex);
			if (m_orderedEnvs.size())
			{
				return m_orderedEnvs.begin()->second;
			}
		}
		return createEnv();
	}

	std::vector<std::shared_ptr<Env>> EnvManager::getAllEnv() const
	{
		std::vector<std::shared_ptr<Env>> result;
		std::shared_lock lock(m_mutex);
		result.reserve(m_orderedEnvs.size());
		for (auto it = m_orderedEnvs.begin(); it != m_orderedEnvs.end(); ++it)
		{
			result.push_back(it->second);
		}
		return result;
	}

	void EnvManager::addEnv(const std::shared_ptr<Env>& env)
	{
		std::unique_lock lock(m_mutex);
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
