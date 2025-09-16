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
import Utility.SystemInfo;

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

	void delete_dll_from_device(std::wstring_view flagName)
	{
		namespace fs = std::filesystem;
		if (const fs::path path32{biz::get_dll_full_path<ArchBit::Bit32>(flagName)}; fs::exists(path32))
		{
			const fs::path tempPath{fs::weakly_canonical(biz::get_bin_path() / fs::path{std::format(L"{}_temp32.bin", flagName)})};
			fs::rename(path32, tempPath);
			fs::remove(tempPath);
		}

		if (const fs::path path64{biz::get_dll_full_path<ArchBit::Bit64>(flagName)}; fs::exists(path64))
		{
			const fs::path tempPath{fs::weakly_canonical(biz::get_bin_path() / fs::path{std::format(L"{}_temp64.bin", flagName)})};
			fs::rename(path64, tempPath);
			fs::remove(tempPath);
		}
	}

	void delete_dir_by_cmd(std::wstring_view dir)
	{
		PROCESS_INFORMATION procInfo = {nullptr};
		STARTUPINFOW startupInfo = {sizeof(startupInfo)};
		startupInfo.dwFlags = STARTF_USESHOWWINDOW;
		startupInfo.wShowWindow = SW_HIDE;
		namespace fs = std::filesystem;
		const fs::path cmdPath{fs::weakly_canonical(fs::path{sys_info::get_system_dir()} / fs::path{L"cmd.exe"})};
		std::wstring cmdLine = std::format(LR"(/c rd /s /q "{}")", dir);
		if (CreateProcessW(cmdPath.c_str(), cmdLine.data(),
		                   nullptr, nullptr, FALSE, CREATE_DEFAULT_ERROR_MODE, nullptr, nullptr,
		                   &startupInfo, &procInfo))
		{
			CloseHandle(procInfo.hThread);
			CloseHandle(procInfo.hProcess);
		}
	}

	void delete_env_dir(std::uint32_t index, std::wstring_view flagName)
	{
		namespace fs = std::filesystem;
		try
		{
			delete_dll_from_device(flagName);

			const fs::path envDir{fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"})};
			const fs::path envPath{fs::weakly_canonical(envDir / fs::path{std::format(L"{}", index)})};
			const fs::path tempPath{fs::weakly_canonical(envDir / fs::path{std::format(L"{}_{}_to_delete", index, flagName)})};

			if (fs::exists(envPath) && !fs::exists(tempPath))
			{
				fs::rename(envPath, tempPath);
				delete_dir_by_cmd(tempPath.native());
			}
		}
		catch (...)
		{
		}
	}
}

namespace biz
{
	EnvManager::EnvManager()
	{
		initialize_env_reg([this](const EnvInitializeData& data)
		{
			loadEnvFrom(data.index, data.flag, data.flagName, data.name);
		});
	}

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
		auto [flag, flagName] = ensureCreateNewEnvFlag(index);
		const std::wstring name = std::format(L"环境{}", index);
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

	void EnvManager::deleteEnv(const std::shared_ptr<Env>& env)
	{
		removeEnv(env->getFlag());
		delete_env_dir(env->getIndex(), env->getFlagName());
		delete_env_from_reg(env->getFlagName());
	}

	bool EnvManager::containsToplevelWindowExcludingByFlag(void* hWnd, std::uint64_t excludeEnvFlag) const
	{
		std::vector<std::shared_ptr<Env>> allEnv = getAllEnv();
		for (const std::shared_ptr<Env>& env : allEnv)
		{
			if (env->getFlag() == excludeEnvFlag)
			{
				continue;
			}
			if (env->containsToplevelWindow(hWnd))
			{
				return true;
			}
		}
		return false;
	}

	std::vector<void*> EnvManager::getAllToplevelWindows() const
	{
		std::vector<std::shared_ptr<Env>> allEnv = getAllEnv();
		std::vector<void*> result;
		result.reserve(allEnv.size() * 4);
		for (const std::shared_ptr<Env>& env : allEnv)
		{
			std::vector<void*> temp = env->getAllToplevelWindows();
			result.insert(result.end(), temp.begin(), temp.end());
		}
		return result;
	}

	std::vector<std::shared_ptr<Env>> EnvManager::getAllEnv() const
	{
		std::vector<std::shared_ptr<Env>> result;
		std::shared_lock lock(m_mutex);
		result.reserve(m_flagToEnv.size());
		for (auto it = m_flagToEnv.begin(); it != m_flagToEnv.end(); ++it)
		{
			result.push_back(it->second);
		}
		return result;
	}

	void EnvManager::setEnvChangeNotify(EnvChangeNotify envChangeNotify)
	{
		std::unique_lock lock(m_mutex);
		m_envChangeNotify = std::move(envChangeNotify);
	}

	EnvManager::EnvFlagInfo EnvManager::ensureCreateNewEnvFlag(std::uint32_t index) const
	{
		namespace fs = std::filesystem;
		const fs::path envPath{fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"} / fs::path{std::format(L"{}", index)})};
		fs::create_directories(envPath);

		EnvFlagInfo result;
		while (true)
		{
			const std::uint64_t flag = get_random_number();
			if (findEnvByFlagNoExcept(flag))
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

	void EnvManager::addEnv(const std::shared_ptr<Env>& env)
	{
		std::unique_lock lock(m_mutex);
		if (!m_flagToEnv.insert(std::make_pair(env->getFlag(), env)).second)
		{
			throw std::runtime_error("add env failed! env flag error!");
		}
		if (m_envChangeNotify)
		{
			m_envChangeNotify(EChangeType::Create, env);
		}
	}

	void EnvManager::removeEnv(std::uint64_t flag)
	{
		std::unique_lock lock(m_mutex);
		const auto it = m_flagToEnv.find(flag);
		if (it == m_flagToEnv.end())
		{
			throw std::runtime_error("remove env failed! can't find env flag!");
		}
		if (m_envChangeNotify)
		{
			m_envChangeNotify(EChangeType::Delete, it->second);
		}
		m_flagToEnv.erase(it);
	}
}
