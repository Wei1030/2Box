export module PELoader:Symbol;

import std;
import "sys_defs.h";

namespace pe
{
	export NtdllSymbolRvaInfo g_sym_rva;
	
	export void fill_all_symbols(const NtdllSymbolRvaInfo& symRva)
	{
		g_sym_rva = symRva;
	}
}
