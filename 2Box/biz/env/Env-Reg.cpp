module Env;

import "sys_defs.h";
#ifndef _SYS_DEFS_H_
#pragma message("Just for IntelliSense. You should not see this message!")
import "sys_defs.hpp";
#endif

import MainApp;

namespace
{
	constexpr wchar_t ENV_KEY_NAME[] = L"Env";
	constexpr wchar_t INDEX_PROP_NAME[] = L"Index";
	constexpr wchar_t FLAG_PROP_NAME[] = L"Flag";
	constexpr wchar_t NAME_PROP_NAME[] = L"Name";

	struct EnvProperty
	{
		std::uint32_t index{0};
		std::uint64_t flag{0};
		std::wstring name;
	};

	EnvProperty get_env_property(HKEY hRootEnvKey, std::wstring_view subKeyName)
	{
		EnvProperty result;
		DWORD dwType = REG_DWORD;
		DWORD dwSize = sizeof(result.index);
		LSTATUS status = RegGetValueW(hRootEnvKey, subKeyName.data(), INDEX_PROP_NAME, RRF_RT_REG_DWORD, &dwType, &result.index, &dwSize);
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to get index, status:{}", status));
		}
		dwType = REG_QWORD;
		dwSize = sizeof(result.flag);
		status = RegGetValueW(hRootEnvKey, subKeyName.data(), FLAG_PROP_NAME, RRF_RT_REG_QWORD, &dwType, &result.flag, &dwSize);
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to get flag, status:{}", status));
		}
		dwType = REG_SZ;
		dwSize = 0;
		status = RegGetValueW(hRootEnvKey, subKeyName.data(), NAME_PROP_NAME, RRF_RT_REG_SZ, &dwType, nullptr, &dwSize);
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to get name length, status:{}", status));
		}
		result.name.resize(dwSize / sizeof(wchar_t));
		status = RegGetValueW(hRootEnvKey, subKeyName.data(), NAME_PROP_NAME, RRF_RT_REG_SZ, &dwType, result.name.data(), &dwSize);
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to get name, status:{}", status));
		}
		return result;
	}

	void set_env_property_to_reg(const biz::Env* env, HKEY hEnvKey)
	{
		DWORD dwType = REG_DWORD;
		const std::uint32_t index = env->getIndex();
		LSTATUS status = RegSetValueExW(hEnvKey, INDEX_PROP_NAME, 0, dwType, reinterpret_cast<const BYTE*>(&index), sizeof(index));
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to set index, status:{}", status));
		}
		dwType = REG_QWORD;
		const std::uint64_t flag = env->getFlag();
		status = RegSetValueExW(hEnvKey, FLAG_PROP_NAME, 0, dwType, reinterpret_cast<const BYTE*>(&flag), sizeof(flag));
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to set flag, status:{}", status));
		}
		dwType = REG_SZ;
		std::wstring_view name = env->getName();
		status = RegSetValueExW(hEnvKey, NAME_PROP_NAME, 0, dwType,
		                        reinterpret_cast<const BYTE*>(name.data()), static_cast<DWORD>((name.size() + 1) * sizeof(wchar_t)));
		if (status != ERROR_SUCCESS)
		{
			throw std::runtime_error(std::format("Failed to set name, status:{}", status));
		}
	}

	std::filesystem::path& get_data_path()
	{
		namespace fs = std::filesystem;
		struct PathWrapper
		{
			PathWrapper()
			{
				dataPath = fs::weakly_canonical(fs::path{app().exeDir()} / fs::path{L"Env\\data"});
				fs::create_directories(dataPath);
			}

			fs::path dataPath;
		};
		static PathWrapper pathWrapper;
		return pathWrapper.dataPath;
	}

	const biz::RegKey& get_app_key()
	{
		namespace fs = std::filesystem;
		struct AppKeyWrapper
		{
			AppKeyWrapper()
			{
				fs::path path = fs::weakly_canonical(get_data_path() / fs::path{MainApp::appName});
				appKey = biz::RegKey{
					[&]()-> HKEY
					{
						HKEY hAppKey;
						if (LSTATUS status = RegLoadAppKeyW(path.native().c_str(), &hAppKey, KEY_ALL_ACCESS, 0, 0);
							status != ERROR_SUCCESS)
						{
							throw std::runtime_error(std::format("RegLoadAppKeyW failed, error code:{}", status));
						}
						return hAppKey;
					}
				};
			}

			biz::RegKey appKey;
		};
		static const AppKeyWrapper appKeyWrapper{};
		return appKeyWrapper.appKey;
	}
}


namespace biz
{
	void initialize_env_reg(const EnvInitializeNotify& notify)
	{
		const RegKey& appKey = get_app_key();
		const RegKey rootEnvKey{
			[&]()-> HKEY
			{
				HKEY hRootEnvKey;
				LSTATUS status = RegCreateKeyExW(appKey, ENV_KEY_NAME,
				                                 0, nullptr, 0, KEY_ALL_ACCESS, nullptr,
				                                 &hRootEnvKey, nullptr);
				if (status != ERROR_SUCCESS)
				{
					throw std::runtime_error(std::format("RegCreateKeyExW failed, error code:{}", status));
				}
				return hRootEnvKey;
			}
		};

		DWORD dwSubKeyCount = 0;
		DWORD dwMaxSubKeyLength = 0;
		LSTATUS status = RegQueryInfoKeyW(rootEnvKey, nullptr, nullptr, nullptr,
		                                  &dwSubKeyCount, &dwMaxSubKeyLength,
		                                  nullptr, nullptr, nullptr, nullptr, nullptr, nullptr);
		if (status != ERROR_SUCCESS)
		{
			return;
		}
		dwMaxSubKeyLength += 1;

		for (DWORD i = 0; i < dwSubKeyCount; i++)
		{
			std::wstring strSubKeyName(dwMaxSubKeyLength, L'\0');
			DWORD subKeyNameLength = dwMaxSubKeyLength;
			status = RegEnumKeyExW(rootEnvKey, i, strSubKeyName.data(), &subKeyNameLength,
			                       nullptr, nullptr, nullptr, nullptr);
			if (status != ERROR_SUCCESS)
			{
				continue;
			}
			strSubKeyName.resize(subKeyNameLength);
			try
			{
				auto [index, flag, name] = get_env_property(rootEnvKey, strSubKeyName);
				notify(EnvInitializeData{index, flag, strSubKeyName, name});
			}
			catch (...)
			{
			}
		}
	}

	void add_env_to_reg(std::wstring_view flagName, const Env* env)
	{
		const RegKey& appKey = get_app_key();
		const RegKey rootEnvKey{
			[&]()-> HKEY
			{
				HKEY hRootEnvKey;
				if (LSTATUS status = RegOpenKeyExW(appKey, ENV_KEY_NAME, 0, KEY_ALL_ACCESS, &hRootEnvKey);
					status != ERROR_SUCCESS)
				{
					throw std::runtime_error(std::format("RegOpenKeyExW failed, error code:{}", status));
				}
				return hRootEnvKey;
			}
		};
		const RegKey newEnvKey{
			[&]()-> HKEY
			{
				HKEY hNewEnvKey;
				LSTATUS status = RegCreateKeyExW(rootEnvKey, flagName.data(),
				                                 0, nullptr, 0, KEY_ALL_ACCESS, nullptr,
				                                 &hNewEnvKey, nullptr);
				if (status != ERROR_SUCCESS)
				{
					throw std::runtime_error(std::format("RegCreateKeyExW failed, error code:{}", status));
				}
				return hNewEnvKey;
			}
		};

		try
		{
			set_env_property_to_reg(env, newEnvKey);
		}
		catch (const std::exception&)
		{
			RegDeleteTreeW(rootEnvKey, flagName.data());
			throw;
		}
		catch (...)
		{
			RegDeleteTreeW(rootEnvKey, flagName.data());
			throw std::runtime_error("unknown error in set_env_property_to_reg");
		}
	}

	void delete_env_from_reg(std::wstring_view flagName)
	{
		const RegKey& appKey = get_app_key();
		const RegKey rootEnvKey{
			[&]()-> HKEY
			{
				HKEY hRootEnvKey;
				if (LSTATUS status = RegOpenKeyExW(appKey, ENV_KEY_NAME, 0, KEY_ALL_ACCESS, &hRootEnvKey);
					status != ERROR_SUCCESS)
				{
					throw std::runtime_error(std::format("RegOpenKeyExW failed, error code:{}", status));
				}
				return hRootEnvKey;
			}
		};
		RegDeleteTreeW(rootEnvKey, flagName.data());
	}
}
