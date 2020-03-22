#include "D3D9Hook.h"
#include <Hooking.Patterns.h>
#include <MinHook.h>
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

	static bool gSwapChainPresentHooked = false;

	static void (*gPreResetCallback)(IDirect3DDevice9*) = nullptr;
	static void (*gPostResetCallback)(IDirect3DDevice9*) = nullptr;
	static void (*gEndSceneCallback)(IDirect3DDevice9*) = nullptr;
	static void (*gPresentCallback)(IDirect3DDevice9*, IDirect3DSwapChain9*) = nullptr;

	static HRESULT(__stdcall* IDirect3DSwapChain9_Present_orig)(IDirect3DSwapChain9* swapChain,
																const RECT* pSourceRect,
																const RECT* pDestRect,
																HWND hDestWindowOverride,
																const RGNDATA* pDirtyRegion,
																DWORD dwFlags);
	static HRESULT __stdcall IDirect3DSwapChain9_Present_detour(IDirect3DSwapChain9* swapChain,
																const RECT* pSourceRect,
																const RECT* pDestRect,
																HWND hDestWindowOverride,
																const RGNDATA* pDirtyRegion,
																DWORD dwFlags)
	{
		if (gPresentCallback)
		{
			if (IDirect3DDevice9* dev = NULL; swapChain->GetDevice(&dev) >= 0 && dev)
			{
				gPresentCallback(dev, swapChain);
				dev->Release();
			}
		}

		return IDirect3DSwapChain9_Present_orig(swapChain,
												pSourceRect,
												pDestRect,
												hDestWindowOverride,
												pDirtyRegion,
												dwFlags);
	}

	static HRESULT(__stdcall* RageDirect3DDevice9_Reset_orig)(RageDirect3DDevice9* rageDevice,
															  D3DPRESENT_PARAMETERS* params);
	static HRESULT __stdcall RageDirect3DDevice9_Reset_detour(RageDirect3DDevice9* rageDevice,
															  D3DPRESENT_PARAMETERS* params)
	{
		if (gPreResetCallback)
		{
			gPreResetCallback(rageDevice->Device());
		}

		const HRESULT r = RageDirect3DDevice9_Reset_orig(rageDevice, params);

		if (gPostResetCallback)
		{
			gPostResetCallback(rageDevice->Device());
		}

		return r;
	}

	static HRESULT(__stdcall* RageDirect3DDevice9_EndScene_orig)(RageDirect3DDevice9* rageDevice);
	static HRESULT __stdcall RageDirect3DDevice9_EndScene_detour(RageDirect3DDevice9* rageDevice)
	{
		if (!gSwapChainPresentHooked)
		{
			if (IDirect3DSwapChain9* swap = nullptr;
				rageDevice->Device()->GetSwapChain(0, &swap) >= 0 && swap)
			{
				constexpr size_t PresentIndex{ 3 };
				void* presentAddr = (*reinterpret_cast<void***>(swap))[PresentIndex];

				MH_CreateHook(presentAddr,
							  &IDirect3DSwapChain9_Present_detour,
							  reinterpret_cast<void**>(&IDirect3DSwapChain9_Present_orig));
				MH_EnableHook(presentAddr);

				swap->Release();

				gSwapChainPresentHooked = true;
			}
		}

		if (gEndSceneCallback)
		{
			gEndSceneCallback(rageDevice->Device());
		}

		return RageDirect3DDevice9_EndScene_orig(rageDevice);
	}

	void init()
	{
		// RageDirect3DDevice9::EndScene
		void* addr =
			hook::get_pattern("8B 44 24 04 8B 80 ? ? ? ? 8B 08 89 44 24 04 FF A1 A8 00 00 00");
		MH_CreateHook(addr,
					  &RageDirect3DDevice9_EndScene_detour,
					  reinterpret_cast<void**>(&RageDirect3DDevice9_EndScene_orig));

		// RageDirect3DDevice9::Reset
		addr = hook::get_pattern("57 B8 ? ? ? ? B9 ? ? ? ? BF ? ? ? ? F3 AB");
		MH_CreateHook(addr,
					  &RageDirect3DDevice9_Reset_detour,
					  reinterpret_cast<void**>(&RageDirect3DDevice9_Reset_orig));
	}

	void set_end_scene_callback(void (*cb)(IDirect3DDevice9*)) { gEndSceneCallback = cb; }
	void set_pre_reset_callback(void (*cb)(IDirect3DDevice9*)) { gPreResetCallback = cb; }
	void set_post_reset_callback(void (*cb)(IDirect3DDevice9*)) { gPostResetCallback = cb; }
	void set_present_callback(void (*cb)(IDirect3DDevice9*, IDirect3DSwapChain9*))
	{
		gPresentCallback = cb;
	}
}
