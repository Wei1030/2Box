export module GlobalData;

import std;

namespace global
{
	export class Data
	{
	public:
		static Data& get()
		{
			static Data instance;
			return instance;
		}

	public:
		void initialize(std::uint64_t envFlag)
		{
			m_envFlag = envFlag;
			m_envFlagName = std::format(L"{:016X}", envFlag);
			m_envFlagNameA = std::format("{:016X}", envFlag);
		}

	public:
		std::uint64_t envFlag() const
		{
			return m_envFlag;
		}

		std::wstring_view envFlagName() const
		{
			return m_envFlagName;
		}

		std::string_view envFlagNameA() const
		{
			return m_envFlagNameA;
		}

	private:
		Data() = default;

	private:
		std::uint64_t m_envFlag{0};
		std::wstring m_envFlagName;
		std::string m_envFlagNameA;
	};
}
