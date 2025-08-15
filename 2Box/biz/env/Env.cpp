module Env;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import std;
import MainApp;
import EssentialData;

namespace biz
{
	void EnvManager::initialize()
	{
		namespace fs = std::filesystem;
		const fs::path envDir{fs::weakly_canonical(fs::path{get_essential_data().envDir})};
		for (const auto& entry : fs::directory_iterator(envDir))
		{
			if (entry.is_regular_file())
			{
				const auto& path = entry.path();
				if (path.extension().empty())
				{
					const auto& stem = path.stem();
					if (!stem.empty())
					{
						std::unique_ptr<Env> env = std::make_unique<Env>();
						env->setPath(path.native());
						m_envMap.insert(std::make_pair(stem.native(), std::move(env)));
					}
				}
			}
		}

		Reg::instance().initialize();
		
		for (auto it = m_envMap.begin(); it != m_envMap.end();)
		{
			Env* env = it->second.get();
			if (env->isValid())
			{
				m_envs.insert(std::make_pair(env->getIndex(), env));
				if (env->getIndex() > m_currentIndex)
				{
					m_currentIndex = env->getIndex();
				}
			}
			else
			{
				std::error_code ec;
				fs::remove(env->getPath(), ec);
				it = m_envMap.erase(it);
				continue;
			}
			++it;
		}
	}

	void Reg::initialize()
	{
		namespace fs = std::filesystem;
		const fs::path regDataPath{fs::weakly_canonical(fs::path{get_essential_data().envDir} / fs::path{L"data"} / fs::path{MainApp::appName})};
		fs::create_directories(regDataPath.parent_path());

		LSTATUS status = RegLoadAppKeyW(regDataPath.native().c_str(), &m_appKey, KEY_ALL_ACCESS, 0, 0);
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("RegLoadAppKeyW failed, error code:{}", status));
		}

		HKEY hRootEnvKey;
		status = RegCreateKeyExW(m_appKey, L"Env", 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hRootEnvKey, nullptr);
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("RegCreateKeyExW failed, error code:{}", status));
		}

		DWORD dwSubKeyCount = 0;
		DWORD dwMaxSubKeyLength = 0;
		status = RegQueryInfoKeyW(hRootEnvKey, nullptr, nullptr, nullptr,
		                          &dwSubKeyCount, &dwMaxSubKeyLength,
		                          nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		if (status != ERROR_SUCCESS)
		{
			return;
		}
		dwMaxSubKeyLength += 1;

		std::vector<std::wstring> envToDelete;
		for (DWORD i = 0; i < dwSubKeyCount; i++)
		{
			std::wstring strSubKeyName(dwMaxSubKeyLength, L'\0');
			DWORD subKeyNameLength = dwMaxSubKeyLength;
			status = RegEnumKeyExW(hRootEnvKey, 0, strSubKeyName.data(), &dwMaxSubKeyLength, nullptr, nullptr, nullptr, nullptr);
			if (status != ERROR_SUCCESS)
			{
				continue;
			}
			strSubKeyName.resize(subKeyNameLength);
			if (Env* env = EnvManager::instance().getEnvByName(strSubKeyName))
			{
				auto getIntValue = [&](std::wstring_view lpValue, DWORD dwFlags, auto* pData)-> LSTATUS
				{
					DWORD cbData = sizeof(*pData);
					return RegGetValueW(hRootEnvKey, strSubKeyName.c_str(), lpValue.data(), dwFlags, nullptr, pData, &cbData);
				};
				DWORD dwIndex = 0;
				LSTATUS indexGetterResult = getIntValue(L"Index", RRF_RT_REG_DWORD, &dwIndex);
				std::uint64_t dwFlag = 0;
				LSTATUS flagGetterResult = getIntValue(L"Flag", RRF_RT_REG_QWORD, &dwFlag);
				if (indexGetterResult == ERROR_SUCCESS && flagGetterResult == ERROR_SUCCESS)
				{
					env->setIndex(dwIndex);
					env->setFlag(dwFlag);
					env->setValid();
					continue;
				}
			}

			envToDelete.push_back(strSubKeyName);
		}

		for (const std::wstring& toDelete : envToDelete)
		{
			RegDeleteTreeW(hRootEnvKey, toDelete.c_str());
		}
	}

	void Reg::addEnv(const std::wstring& name, const Env* env) const
	{
		HKEY hRootEnvKey;
		LSTATUS status = RegOpenKeyExW(m_appKey, L"Env", 0, KEY_ALL_ACCESS, &hRootEnvKey);
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("RegOpenKeyExW failed, error code:{}", status));
		}
		HKEY hNewEnvKey;
		status = RegCreateKeyExW(hRootEnvKey, name.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hNewEnvKey, nullptr);
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("RegCreateKeyExW failed, error code:{}", status));
		}
		auto setIntValue = [&](std::wstring_view lpValue, DWORD dwType, const auto* pData)-> LSTATUS
		{
			return RegSetValueExW(hNewEnvKey, lpValue.data(), 0, dwType, reinterpret_cast<const BYTE*>(pData), sizeof(*pData));
		};
		DWORD index = env->getIndex();
		LSTATUS indexSetterResult = status = setIntValue(L"Index", REG_DWORD, &index);
		std::uint64_t flag = env->getFlag();
		LSTATUS flagSetterResult = setIntValue(L"Flag", REG_QWORD, &flag);
		if (indexSetterResult != ERROR_SUCCESS || flagSetterResult != ERROR_SUCCESS)
		{
			RegDeleteTreeW(hRootEnvKey, name.c_str());
			throw std::runtime_error(std::format("RegSetValue failed, indexSetterResult:{}, flagSetterResult:{}", indexSetterResult, flagSetterResult));
		}
	}
}
