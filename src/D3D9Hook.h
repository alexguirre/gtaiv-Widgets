#pragma once
#include <d3d9.h>

namespace d3d9_hook
{
	void init();
	void set_end_scene_callback(void (*cb)(IDirect3DDevice9*));
	void set_pre_reset_callback(void (*cb)(IDirect3DDevice9*));
	void set_post_reset_callback(void (*cb)(IDirect3DDevice9*));
	void set_present_callback(void (*cb)(IDirect3DDevice9*, IDirect3DSwapChain9*));
}
