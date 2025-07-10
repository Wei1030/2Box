export module UI.MainPage:Define;

import std;
import UI.PageBase;

namespace ui
{
	export enum class MainPageType : std::uint8_t
	{
		Download,
		Home,
		TotalCount
	};

	export
	template <typename PageEnumType, PageEnumType>
	class TMainPageType : public PageBase
	{
	};
}
