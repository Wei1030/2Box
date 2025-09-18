export module Utility.LiteralName;

import std;

namespace utils
{
	template <typename CharType, size_t N>
	struct StringLiteral
	{
		using Type = CharType;

		consteval StringLiteral(const CharType (&str)[N])
		{
			for (size_t i = 0; i < N; ++i)
			{
				data[i] = str[i];
			}
		}

		CharType data[N];

		static constexpr size_t size = N;
	};

	export
	template <typename CharType, CharType... C>
	struct LiteralName
	{
		using Type = CharType;
		static constexpr std::size_t size = sizeof...(C);
		static constexpr CharType data[size] = {C...};
	};

	template <std::size_t Index, typename CharType, std::size_t N>
	consteval CharType get_char_from_str(const CharType (&arr)[N])
	{
		return arr[Index < N ? Index : N - 1];
	}

	template <StringLiteral Str, size_t... N>
	consteval auto make_literal_name_helper(std::index_sequence<N...>)
	{
		return LiteralName<typename decltype(Str)::Type, get_char_from_str<N>(Str.data)...>{};
	}


	export template <StringLiteral Str>
	consteval auto make_literal_name()
	{
		return make_literal_name_helper<Str>(std::make_index_sequence<Str.size>{});
	}
}
