#include "D3D9Hook.h"
#include <Hooking.Patterns.h>
#include <cstdint>
#include <d3d9.h>
#include <spdlog/spdlog.h>

namespace d3d9_hook
{
	struct RageDirect3DDevice9
	{
		IDirect3DDevice9* Device()
		{
			return *reinterpret_cast<IDirect3DDevice9**>(reinterpret_cast<char*>(this) + 0x11AC);
		}
	};

	static void RageDirect3DDevice9_EndScene_detour(RageDirect3DDevice9* rageDevice)
	{
		spdlog::debug("RageDirect3DDevice9::EndScene({})", reinterpret_cast<void*>(rageDevice));
		rageDevice->Device()->EndScene();
	}

	void init()
	{
		// RageDirect3DDevice9::EndScene
		void* addr =
			hook::get_pattern("8B 44 24 04 8B 80 ? ? ? ? 8B 08 89 44 24 04 FF A1 A8 00 00 00");

		uint8_t patch[]{
			0x68, 0x00, 0x00, 0x00, 0x00, // push 0x00000000
			0xC3                          // retn
		};

		// write the address of the detour in the push instruction value
		*reinterpret_cast<void**>(&patch[1]) = RageDirect3DDevice9_EndScene_detour;

		DWORD oldProtect;
		VirtualProtect(addr, sizeof(patch), PAGE_EXECUTE_READWRITE, &oldProtect);
		memcpy(addr, patch, sizeof(patch));
		VirtualProtect(addr, sizeof(patch), oldProtect, &oldProtect);
	}

}
