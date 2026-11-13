module Env;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import std;
import MainApp;
import UI.MainWindow;
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

	// 返回 Env/<index> 目录路径
	std::filesystem::path get_env_dir_path(std::uint32_t index)
	{
		namespace fs = std::filesystem;
		return fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"} / fs::path{std::format(L"{}", index)});
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
		namespace fs = std::filesystem;
		const fs::path envPath{fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env"} / fs::path{std::format(L"{}", index)})};
		fs::create_directories(envPath);
		addEnv(std::make_shared<Env>(index, flag, flagName, name));
	}

	std::shared_ptr<Env> EnvManager::createEnv()
	{
		if (getEnvCount() >= 100)
		{
			throw std::runtime_error("最多只能创建100个环境");
		}

		// 优先复用被删除容器留下的最小空闲编号，保证编号连续（例如删除10号后再次创建仍得到10号）
		std::uint32_t index = 0;
		{
			std::unique_lock lock(m_mutex);
			while (m_usedIndices.contains(index))
			{
				++index;
			}
			// 先预留编号，避免并发创建时重复使用
			m_usedIndices.insert(index);
		}

		try
		{
			auto [flag, flagName] = ensureCreateNewEnvFlag(index);
			const std::wstring name = std::format(L"环境{}", index);
			std::shared_ptr<Env> envResult = std::make_shared<Env>(index, flag, flagName, name);
			add_env_to_reg(flagName, envResult.get());
			addEnv(envResult);
			return envResult;
		}
		catch (...)
		{
			// 创建失败时释放预留的编号
			std::unique_lock lock(m_mutex);
			m_usedIndices.erase(index);
			throw;
		}
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

	std::size_t EnvManager::getEnvCount() const
	{
		std::shared_lock lock(m_mutex);
		return m_flagToEnv.size();
	}

	// ReSharper disable once CppPassValueParameterByConstReference
	void EnvManager::deleteEnv(std::shared_ptr<Env> env)
	{
		if (!env)
		{
			throw std::invalid_argument("env is null");
		}
		if (env->getAllProcessesCount() != 0)
		{
			throw std::runtime_error(std::format("环境{}中仍有正在运行的进程，无法删除", env->getIndex()));
		}

		// 删除后需要把编号更大的容器依次前移以保持编号连续。
		// 这些容器中不能有运行中的进程：它们的进程已按旧编号/旧目录完成文件重定向，改名会破坏其运行状态。
		std::vector<std::shared_ptr<Env>> envsToRenumber;
		{
			std::shared_lock lock(m_mutex);
			for (const auto& item : m_flagToEnv)
			{
				const std::shared_ptr<Env>& other = item.second;
				if (other.get() != env.get() && other->getIndex() > env->getIndex())
				{
					envsToRenumber.push_back(other);
				}
			}
		}
		for (const std::shared_ptr<Env>& other : envsToRenumber)
		{
			if (other->getAllProcessesCount() != 0)
			{
				throw std::runtime_error(std::format("无法保持编号连续：环境{}中仍有正在运行的进程，请先关闭后再删除", other->getIndex()));
			}
		}
		std::sort(envsToRenumber.begin(), envsToRenumber.end(),
		          [](const std::shared_ptr<Env>& lhs, const std::shared_ptr<Env>& rhs) { return lhs->getIndex() < rhs->getIndex(); });

		const std::uint32_t deletedIndex = env->getIndex();
		env->deleteDllFromDevice();
		removeEnv(env->getFlag());
		delete_env_dir(deletedIndex, env->getFlagName());
		delete_env_from_reg(env->getFlagName());

		if (!envsToRenumber.empty())
		{
			renumberEnvsAfter(envsToRenumber, deletedIndex);

			// 同步编号集合，并通知界面重建卡片索引
			std::unique_lock lock(m_mutex);
			m_usedIndices.clear();
			for (const auto& item : m_flagToEnv)
			{
				m_usedIndices.insert(item.second->getIndex());
			}
			if (m_envChangeNotify)
			{
				m_envChangeNotify(EChangeType::Renumber, env);
			}
		}
	}

	void EnvManager::renumberEnvsAfter(const std::vector<std::shared_ptr<Env>>& envsToRenumber, std::uint32_t deletedIndex)
	{
		namespace fs = std::filesystem;

		struct RenumberMove
		{
			std::shared_ptr<Env> env;
			std::uint32_t oldIndex{0};
			std::uint32_t newIndex{0};
			fs::path oldPath;
			fs::path tempPath;
			fs::path newPath;
		};

		std::vector<RenumberMove> moves;
		moves.reserve(envsToRenumber.size());
		std::uint32_t nextIndex = deletedIndex;
		for (const std::shared_ptr<Env>& env : envsToRenumber)
		{
			RenumberMove move;
			move.env = env;
			move.oldIndex = env->getIndex();
			move.newIndex = nextIndex++;
			move.oldPath = get_env_dir_path(move.oldIndex);
			move.tempPath = move.oldPath.parent_path() / fs::path{std::format(L"{}_{}_renumbering", move.oldIndex, env->getFlagName())};
			move.newPath = get_env_dir_path(move.newIndex);
			moves.push_back(std::move(move));
		}

		// 第一阶段：旧编号目录 -> 临时目录，避免两两改名时的目标目录冲突
		std::size_t movedToTemp = 0;
		try
		{
			for (; movedToTemp < moves.size(); ++movedToTemp)
			{
				const RenumberMove& move = moves[movedToTemp];
				if (fs::exists(move.oldPath))
				{
					fs::rename(move.oldPath, move.tempPath);
				}
			}
		}
		catch (...)
		{
			for (std::size_t i = movedToTemp; i > 0; --i)
			{
				try
				{
					if (fs::exists(moves[i - 1].tempPath))
					{
						fs::rename(moves[i - 1].tempPath, moves[i - 1].oldPath);
					}
				}
				catch (...)
				{
				}
			}
			throw std::runtime_error("自动调整环境编号失败：无法重命名环境目录，请重试");
		}

		// 第二阶段：临时目录 -> 新编号目录
		std::size_t placed = 0;
		try
		{
			for (; placed < moves.size(); ++placed)
			{
				const RenumberMove& move = moves[placed];
				if (fs::exists(move.tempPath))
				{
					fs::rename(move.tempPath, move.newPath);
				}
			}
		}
		catch (...)
		{
			for (std::size_t i = placed; i > 0; --i)
			{
				try
				{
					if (fs::exists(moves[i - 1].newPath))
					{
						fs::rename(moves[i - 1].newPath, moves[i - 1].tempPath);
					}
				}
				catch (...)
				{
				}
			}
			for (std::size_t i = moves.size(); i > 0; --i)
			{
				try
				{
					if (fs::exists(moves[i - 1].tempPath))
					{
						fs::rename(moves[i - 1].tempPath, moves[i - 1].oldPath);
					}
				}
				catch (...)
				{
				}
			}
			throw std::runtime_error(std::format("自动调整环境编号失败：目录 Env\\{} 已存在或无法访问", moves[placed].newIndex));
		}

		// 最后更新内存编号与注册表中的 Index
		std::size_t updated = 0;
		try
		{
			for (; updated < moves.size(); ++updated)
			{
				RenumberMove& move = moves[updated];
				set_env_index_in_reg(move.env->getFlagName(), move.newIndex);
				move.env->setIndex(move.newIndex);
			}
		}
		catch (...)
		{
			for (std::size_t i = 0; i < updated; ++i)
			{
				try
				{
					set_env_index_in_reg(moves[i].env->getFlagName(), moves[i].oldIndex);
				}
				catch (...)
				{
				}
				moves[i].env->setIndex(moves[i].oldIndex);
			}
			for (std::size_t i = moves.size(); i > 0; --i)
			{
				try
				{
					if (fs::exists(moves[i - 1].newPath))
					{
						fs::rename(moves[i - 1].newPath, moves[i - 1].oldPath);
					}
				}
				catch (...)
				{
				}
			}
			throw std::runtime_error("自动调整环境编号失败：无法写入注册表，请重试");
		}
	}

	void EnvManager::renameEnv(const std::shared_ptr<Env>& env, std::wstring_view newName)
	{
		if (!env)
		{
			throw std::invalid_argument("env is null");
		}

		std::wstring name{newName};
		constexpr std::wstring_view trimmedChars = L" \t\r\n";
		const std::size_t firstPos = name.find_first_not_of(trimmedChars);
		if (firstPos == std::wstring::npos)
		{
			throw std::invalid_argument("环境名称不能为空");
		}
		const std::size_t lastPos = name.find_last_not_of(trimmedChars);
		name = name.substr(firstPos, lastPos - firstPos + 1);

		if (name.size() > ENV_NAME_MAX_LENGTH)
		{
			throw std::invalid_argument(std::format("环境名称长度不能超过{}个字符", ENV_NAME_MAX_LENGTH));
		}
		if (std::any_of(name.begin(), name.end(), [](wchar_t c) { return std::iswcntrl(static_cast<std::wint_t>(c)) != 0; }))
		{
			throw std::invalid_argument("环境名称不能包含控制字符");
		}
		if (name == env->getName())
		{
			return;
		}

		// 先持久化，成功后再更新内存，避免注册表中保存的名称与界面显示不一致
		set_env_name_in_reg(env->getFlagName(), name);
		env->setName(name);
	}

	bool EnvManager::containsProcessIdExclude(std::uint32_t pid, std::uint64_t excludeEnvFlag) const
	{
		if (GetCurrentProcessId() == pid)
		{
			return true;
		}
		std::vector<std::shared_ptr<Env>> allEnv = getAllEnv();
		for (const std::shared_ptr<Env>& env : allEnv)
		{
			if (env->getFlag() == excludeEnvFlag)
			{
				continue;
			}
			if (env->getProcess(pid))
			{
				return true;
			}
		}
		return false;
	}

	std::vector<DWORD> EnvManager::getAllProcessIdsExclude(std::uint64_t excludeEnvFlag) const
	{
		std::vector<std::shared_ptr<Env>> allEnv = getAllEnv();
		std::vector<DWORD> result;
		result.reserve(allEnv.size() * 2);
		for (const std::shared_ptr<Env>& env : allEnv)
		{
			if (env->getFlag() == excludeEnvFlag)
			{
				continue;
			}
			std::vector<DWORD> temp = env->getAllProcessIds();
			result.insert(result.end(), temp.begin(), temp.end());
		}
		result.push_back(GetCurrentProcessId());
		return result;
	}

	bool EnvManager::containsToplevelWindowExclude(void* hWnd, std::uint64_t excludeEnvFlag) const
	{
		if (hWnd == ui::main_wnd().nativeHandle())
		{
			return true;
		}
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

	std::vector<void*> EnvManager::getAllToplevelWindowsExclude(std::uint64_t excludeEnvFlag) const
	{
		std::vector<std::shared_ptr<Env>> allEnv = getAllEnv();
		std::vector<void*> result;
		result.reserve(allEnv.size() * 4);
		for (const std::shared_ptr<Env>& env : allEnv)
		{
			if (env->getFlag() == excludeEnvFlag)
			{
				continue;
			}
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
		m_usedIndices.insert(env->getIndex());
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

		// 同步已占用编号集合，使被删除容器的编号可以被后续创建复用
		m_usedIndices.clear();
		for (const auto& item : m_flagToEnv)
		{
			m_usedIndices.insert(item.second->getIndex());
		}
	}
}
