export module Env:Envrironment;

import std;

namespace biz
{
	export class Env
	{
	public:
		Env(std::uint32_t index, std::uint64_t flag,
		    std::wstring_view flagName, std::wstring_view name, std::string_view dllFullPath)
			: m_index(index), m_flag(flag), m_flagName(flagName), m_name(name), m_dllFullPath(dllFullPath)
		{
		}

	public:
		std::uint32_t getIndex() const { return m_index; }
		std::uint64_t getFlag() const { return m_flag; }
		std::wstring_view getFlagName() const { return m_flagName; }
		std::wstring_view getName() const { return m_name; }
		std::string_view getDllFullPath() const { return m_dllFullPath; }

	private:
		std::uint32_t m_index{0};
		std::uint64_t m_flag{0};
		std::wstring m_flagName;
		std::wstring m_name;
		std::string m_dllFullPath;
	};
}
