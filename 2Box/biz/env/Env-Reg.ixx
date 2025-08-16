export module Env:Reg;

import "sys_defs.h";
import std;
import :Envrironment;

namespace biz
{
	class RegKey
	{
	public:
		RegKey() = default;
		explicit RegKey(auto creator)
		{
			m_key = creator();
		}

		~RegKey()
		{
			if (m_key)
			{
				RegCloseKey(m_key);
			}
		}

		RegKey(const RegKey&) = delete;
		RegKey& operator=(const RegKey&) = delete;

		RegKey(RegKey&& that) noexcept : m_key(std::exchange(that.m_key, nullptr))
		{
		}

		RegKey& operator=(RegKey&& that) noexcept
		{
			std::swap(m_key, that.m_key);
			return *this;
		}

		operator HKEY() const { return m_key; }

	private:
		HKEY m_key{nullptr};
	};

	export void initialize_env_reg();
	export void add_env_to_reg(std::wstring_view flagName, const Env* env);
}
