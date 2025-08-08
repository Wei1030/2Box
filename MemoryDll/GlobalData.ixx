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
		}
	public:
		std::uint64_t envFlag() const
		{
			return m_envFlag;
		}
	private:
		Data() = default;

	private:
		std::uint64_t m_envFlag{0};
	};
}
