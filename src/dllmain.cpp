#include <Hooking.Patterns.h>
#include <Windows.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

BOOL APIENTRY DllMain([[maybe_unused]] HMODULE hModule,
					  [[maybe_unused]] DWORD ul_reason_for_call,
					  [[maybe_unused]] LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_PROCESS_DETACH: break;
	}
	return TRUE;
}
