export module PELoader:Symbol;

import std;
import "sys_defs.h";

namespace pe
{
	export NtdllSymbolRvaInfo g_sym_rva32;
	export NtdllSymbolRvaInfo g_sym_rva64;
	
	export void fill_all_symbols(const NtdllSymbolRvaInfo& symRva32, const NtdllSymbolRvaInfo& symRva64)
	{
		g_sym_rva32 = symRva32;
		g_sym_rva64 = symRva64;
	}
}
