import std;
import "sys_defs.h";

void test_throw()
{
// 	try
// 	{
// 		throw std::runtime_error("test throw");
// 	}
// 	catch (const std::exception& e)
// 	{
// 		std::cout << e.what() << "\n";
// 	}
}
BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
	std::cout << "Injected!" << "\n";
	test_throw();
	
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}
