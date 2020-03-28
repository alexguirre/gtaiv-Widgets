#include "D3D9ImGui.h"
#include "D3D9Hook.h"
#include <Hooking.Patterns.h>
#include <MinHook.h>
#include <imgui.h>
#include <spdlog/spdlog.h>

namespace d3d9_imgui
{
	static void (*gDrawCallback)();

	static HWND g_hWnd = NULL;
	static INT64 g_Time = 0;
	static INT64 g_TicksPerSecond = 0;

	static LPDIRECT3DDEVICE9 g_pd3dDevice = NULL;
	static LPDIRECT3DVERTEXBUFFER9 g_pVB = NULL;
	static LPDIRECT3DINDEXBUFFER9 g_pIB = NULL;
	static LPDIRECT3DTEXTURE9 g_FontTexture = NULL;
	static int g_VertexBufferSize = 5000, g_IndexBufferSize = 10000;

	struct CUSTOMVERTEX
	{
		float pos[3];
		D3DCOLOR col;
		float uv[2];
	};
#define D3DFVF_CUSTOMVERTEX (D3DFVF_XYZ | D3DFVF_DIFFUSE | D3DFVF_TEX1)

	static LRESULT win32_wnd_proc_handler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		if (ImGui::GetCurrentContext() == NULL)
			return 0;

		ImGuiIO& io = ImGui::GetIO();
		switch (msg)
		{
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			if (wParam < 256)
				io.KeysDown[wParam] = 1;
			return 0;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			if (wParam < 256)
				io.KeysDown[wParam] = 0;
			return 0;
		case WM_CHAR:
			// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
			io.AddInputCharacter((unsigned int)wParam);
			return 0;
		}
		return 0;
	}

	static LRESULT(WINAPI* WndProc_orig)(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	static LRESULT WINAPI WndProc_detour(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
		win32_wnd_proc_handler(hWnd, msg, wParam, lParam);

		return WndProc_orig(hWnd, msg, wParam, lParam);
	}

	static void win32_init(HWND hwnd)
	{
		if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&g_TicksPerSecond))
			return;
		if (!::QueryPerformanceCounter((LARGE_INTEGER*)&g_Time))
			return;

		// Setup back-end capabilities flags
		g_hWnd = (HWND)hwnd;
		ImGuiIO& io = ImGui::GetIO();
		io.BackendPlatformName = "imgui_impl_win32";
		io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
		io.ImeWindowHandle = hwnd;
		io.MouseDrawCursor = true;
		io.ConfigWindowsResizeFromEdges = true;

		// Keyboard mapping. ImGui will use those indices to peek into the io.KeysDown[] array that
		// we will update during the application lifetime.
		io.KeyMap[ImGuiKey_Tab] = VK_TAB;
		io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
		io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
		io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
		io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
		io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
		io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
		io.KeyMap[ImGuiKey_Home] = VK_HOME;
		io.KeyMap[ImGuiKey_End] = VK_END;
		io.KeyMap[ImGuiKey_Insert] = VK_INSERT;
		io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
		io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
		io.KeyMap[ImGuiKey_Space] = VK_SPACE;
		io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
		io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
		io.KeyMap[ImGuiKey_KeyPadEnter] = VK_RETURN;
		io.KeyMap[ImGuiKey_A] = 'A';
		io.KeyMap[ImGuiKey_C] = 'C';
		io.KeyMap[ImGuiKey_V] = 'V';
		io.KeyMap[ImGuiKey_X] = 'X';
		io.KeyMap[ImGuiKey_Y] = 'Y';
		io.KeyMap[ImGuiKey_Z] = 'Z';
	}

	static void dx9_init(IDirect3DDevice9* device)
	{
		ImGuiIO& io = ImGui::GetIO();
		io.BackendRendererName = "imgui_impl_dx9";
		io.BackendFlags |=
			ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset field,
													// allowing for large meshes.

		g_pd3dDevice = device;
	}

	static void dx9_create_fonts_texture()
	{
		// Build texture atlas
		ImGuiIO& io = ImGui::GetIO();
		unsigned char* pixels;
		int width, height, bytes_per_pixel;
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height, &bytes_per_pixel);

		// Upload texture to graphics system
		g_FontTexture = NULL;
		if (g_pd3dDevice->CreateTexture(width,
										height,
										1,
										D3DUSAGE_DYNAMIC,
										D3DFMT_A8R8G8B8,
										D3DPOOL_DEFAULT,
										&g_FontTexture,
										NULL) < 0)
			return;
		D3DLOCKED_RECT tex_locked_rect;
		if (g_FontTexture->LockRect(0, &tex_locked_rect, NULL, 0) != D3D_OK)
			return;
		for (int y = 0; y < height; y++)
			memcpy((unsigned char*)tex_locked_rect.pBits + tex_locked_rect.Pitch * y,
				   pixels + (width * bytes_per_pixel) * y,
				   (width * bytes_per_pixel));
		g_FontTexture->UnlockRect(0);

		// Store our identifier
		io.Fonts->TexID = (ImTextureID)g_FontTexture;
	}

	static void dx9_create_device_objects()
	{
		if (!g_pd3dDevice)
			return;

		dx9_create_fonts_texture();
	}

	static void dx9_invalidate_device_objects()
	{
		if (!g_pd3dDevice)
			return;
		if (g_pVB)
		{
			g_pVB->Release();
			g_pVB = NULL;
		}
		if (g_pIB)
		{
			g_pIB->Release();
			g_pIB = NULL;
		}
		if (g_FontTexture)
		{
			g_FontTexture->Release();
			g_FontTexture = NULL;
			ImGui::GetIO().Fonts->TexID = NULL; // We copied g_pFontTextureView to io.Fonts->TexID
												// so let's clear that as well.
		}
	}

	static void win32_update_mouse_pos()
	{
		ImGuiIO& io = ImGui::GetIO();

		// Set OS mouse position if requested (rarely used, only when
		// ImGuiConfigFlags_NavEnableSetMousePos is enabled by user)
		if (io.WantSetMousePos)
		{
			POINT pos = { (int)io.MousePos.x, (int)io.MousePos.y };
			::ClientToScreen(g_hWnd, &pos);
			::SetCursorPos(pos.x, pos.y);
		}

		// Set mouse position
		io.MousePos = ImVec2(-FLT_MAX, -FLT_MAX);
		POINT pos;
		if (HWND active_window = ::GetForegroundWindow())
			if (active_window == g_hWnd || ::IsChild(active_window, g_hWnd))
				if (::GetCursorPos(&pos) && ::ScreenToClient(g_hWnd, &pos))
					io.MousePos = ImVec2((float)pos.x, (float)pos.y);
	}

	static void win32_new_frame()
	{
		ImGuiIO& io = ImGui::GetIO();
		IM_ASSERT(io.Fonts->IsBuilt() &&
				  "Font atlas not built! It is generally built by the renderer back-end. Missing "
				  "call to renderer _NewFrame() function? e.g. ImGui_ImplOpenGL3_NewFrame().");

		// Setup display size (every frame to accommodate for window resizing)
		RECT rect;
		::GetClientRect(g_hWnd, &rect);
		io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

		// Setup time step
		INT64 current_time;
		::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
		io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
		g_Time = current_time;

		// Read keyboard modifiers inputs
		io.KeyCtrl = (::GetKeyState(VK_CONTROL) & 0x8000) != 0;
		io.KeyShift = (::GetKeyState(VK_SHIFT) & 0x8000) != 0;
		io.KeyAlt = (::GetKeyState(VK_MENU) & 0x8000) != 0;
		io.KeySuper = false;

		io.MouseDown[0] = (::GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
		io.MouseDown[1] = (::GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
		io.MouseDown[2] = (::GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
		io.MouseDown[3] = (::GetAsyncKeyState(VK_XBUTTON1) & 0x8000) != 0;
		io.MouseDown[4] = (::GetAsyncKeyState(VK_XBUTTON2) & 0x8000) != 0;

		// Update OS mouse position
		win32_update_mouse_pos();
	}

	static void dx9_new_frame()
	{
		if (!g_FontTexture)
			dx9_create_device_objects();
	}

	static void dx9_setup_render_state(ImDrawData* draw_data)
	{
		// Setup viewport
		D3DVIEWPORT9 vp;
		vp.X = vp.Y = 0;
		vp.Width = (DWORD)draw_data->DisplaySize.x;
		vp.Height = (DWORD)draw_data->DisplaySize.y;
		vp.MinZ = 0.0f;
		vp.MaxZ = 1.0f;
		g_pd3dDevice->SetViewport(&vp);

		// Setup render state: fixed-pipeline, alpha-blending, no face culling, no depth testing,
		// shade mode (for gradient)
		g_pd3dDevice->SetPixelShader(NULL);
		g_pd3dDevice->SetVertexShader(NULL);
		g_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
		g_pd3dDevice->SetRenderState(D3DRS_LIGHTING, false);
		g_pd3dDevice->SetRenderState(D3DRS_ZENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, true);
		g_pd3dDevice->SetRenderState(D3DRS_ALPHATESTENABLE, false);
		g_pd3dDevice->SetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD);
		g_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
		g_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
		g_pd3dDevice->SetRenderState(D3DRS_SCISSORTESTENABLE, true);
		g_pd3dDevice->SetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD);
		g_pd3dDevice->SetRenderState(D3DRS_FOGENABLE, false);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_DIFFUSE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
		g_pd3dDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);
		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		g_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

		// Setup orthographic projection matrix
		// Our visible imgui space lies from draw_data->DisplayPos (top left) to
		// draw_data->DisplayPos+data_data->DisplaySize (bottom right). DisplayPos is (0,0) for
		// single viewport apps. Being agnostic of whether <d3dx9.h> or <DirectXMath.h> can be used,
		// we aren't relying on D3DXMatrixIdentity()/D3DXMatrixOrthoOffCenterLH() or
		// DirectX::XMMatrixIdentity()/DirectX::XMMatrixOrthographicOffCenterLH()
		{
			float L = draw_data->DisplayPos.x + 0.5f;
			float R = draw_data->DisplayPos.x + draw_data->DisplaySize.x + 0.5f;
			float T = draw_data->DisplayPos.y + 0.5f;
			float B = draw_data->DisplayPos.y + draw_data->DisplaySize.y + 0.5f;
			D3DMATRIX mat_identity = { { { 1.0f,
										   0.0f,
										   0.0f,
										   0.0f,
										   0.0f,
										   1.0f,
										   0.0f,
										   0.0f,
										   0.0f,
										   0.0f,
										   1.0f,
										   0.0f,
										   0.0f,
										   0.0f,
										   0.0f,
										   1.0f } } };
			D3DMATRIX mat_projection = { { { 2.0f / (R - L),
											 0.0f,
											 0.0f,
											 0.0f,
											 0.0f,
											 2.0f / (T - B),
											 0.0f,
											 0.0f,
											 0.0f,
											 0.0f,
											 0.5f,
											 0.0f,
											 (L + R) / (L - R),
											 (T + B) / (B - T),
											 0.5f,
											 1.0f } } };
			g_pd3dDevice->SetTransform(D3DTS_WORLD, &mat_identity);
			g_pd3dDevice->SetTransform(D3DTS_VIEW, &mat_identity);
			g_pd3dDevice->SetTransform(D3DTS_PROJECTION, &mat_projection);
		}
	}

	static void dx9_render_draw_data(ImDrawData* draw_data)
	{ // Avoid rendering when minimized
		if (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f)
			return;

		// Create and grow buffers if needed
		if (!g_pVB || g_VertexBufferSize < draw_data->TotalVtxCount)
		{
			if (g_pVB)
			{
				g_pVB->Release();
				g_pVB = NULL;
			}
			g_VertexBufferSize = draw_data->TotalVtxCount + 5000;
			if (g_pd3dDevice->CreateVertexBuffer(g_VertexBufferSize * sizeof(CUSTOMVERTEX),
												 D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
												 D3DFVF_CUSTOMVERTEX,
												 D3DPOOL_DEFAULT,
												 &g_pVB,
												 NULL) < 0)
				return;
		}
		if (!g_pIB || g_IndexBufferSize < draw_data->TotalIdxCount)
		{
			if (g_pIB)
			{
				g_pIB->Release();
				g_pIB = NULL;
			}
			g_IndexBufferSize = draw_data->TotalIdxCount + 10000;
			if (g_pd3dDevice->CreateIndexBuffer(g_IndexBufferSize * sizeof(ImDrawIdx),
												D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
												sizeof(ImDrawIdx) == 2 ? D3DFMT_INDEX16 :
																		 D3DFMT_INDEX32,
												D3DPOOL_DEFAULT,
												&g_pIB,
												NULL) < 0)
				return;
		}

		// Copy and convert all vertices into a single contiguous buffer, convert colors to DX9
		// default format.
		// FIXME-OPT: This is a waste of resource, the ideal is to use imconfig.h and
		//  1) to avoid repacking colors:   #define IMGUI_USE_BGRA_PACKED_COLOR
		//  2) to avoid repacking vertices: #define IMGUI_OVERRIDE_DRAWVERT_STRUCT_LAYOUT struct
		//  ImDrawVert { ImVec2 pos; float z; ImU32 col; ImVec2 uv; }
		CUSTOMVERTEX* vtx_dst;
		ImDrawIdx* idx_dst;
		if (g_pVB->Lock(0,
						(UINT)(draw_data->TotalVtxCount * sizeof(CUSTOMVERTEX)),
						(void**)&vtx_dst,
						D3DLOCK_DISCARD) < 0)
			return;
		if (g_pIB->Lock(0,
						(UINT)(draw_data->TotalIdxCount * sizeof(ImDrawIdx)),
						(void**)&idx_dst,
						D3DLOCK_DISCARD) < 0)
			return;
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			const ImDrawVert* vtx_src = cmd_list->VtxBuffer.Data;
			for (int i = 0; i < cmd_list->VtxBuffer.Size; i++)
			{
				vtx_dst->pos[0] = vtx_src->pos.x;
				vtx_dst->pos[1] = vtx_src->pos.y;
				vtx_dst->pos[2] = 0.0f;
				vtx_dst->col = (vtx_src->col & 0xFF00FF00) | ((vtx_src->col & 0xFF0000) >> 16) |
							   ((vtx_src->col & 0xFF) << 16); // RGBA --> ARGB for DirectX9
				vtx_dst->uv[0] = vtx_src->uv.x;
				vtx_dst->uv[1] = vtx_src->uv.y;
				vtx_dst++;
				vtx_src++;
			}
			memcpy(idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof(ImDrawIdx));
			idx_dst += cmd_list->IdxBuffer.Size;
		}
		g_pVB->Unlock();
		g_pIB->Unlock();
		g_pd3dDevice->SetStreamSource(0, g_pVB, 0, sizeof(CUSTOMVERTEX));
		g_pd3dDevice->SetIndices(g_pIB);
		g_pd3dDevice->SetFVF(D3DFVF_CUSTOMVERTEX);

		// Setup desired DX state
		dx9_setup_render_state(draw_data);

		// Render command lists
		// (Because we merged all buffers into a single one, we maintain our own offset into them)
		int global_vtx_offset = 0;
		int global_idx_offset = 0;
		ImVec2 clip_off = draw_data->DisplayPos;
		for (int n = 0; n < draw_data->CmdListsCount; n++)
		{
			const ImDrawList* cmd_list = draw_data->CmdLists[n];
			for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++)
			{
				const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
				if (pcmd->UserCallback != NULL)
				{
					// User callback, registered via ImDrawList::AddCallback()
					// (ImDrawCallback_ResetRenderState is a special callback value used by the user
					// to request the renderer to reset render state.)
					if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
						dx9_setup_render_state(draw_data);
					else
						pcmd->UserCallback(cmd_list, pcmd);
				}
				else
				{
					const RECT r = { (LONG)(pcmd->ClipRect.x - clip_off.x),
									 (LONG)(pcmd->ClipRect.y - clip_off.y),
									 (LONG)(pcmd->ClipRect.z - clip_off.x),
									 (LONG)(pcmd->ClipRect.w - clip_off.y) };
					const LPDIRECT3DTEXTURE9 texture = (LPDIRECT3DTEXTURE9)pcmd->TextureId;
					g_pd3dDevice->SetTexture(0, texture);
					g_pd3dDevice->SetScissorRect(&r);
					g_pd3dDevice->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,
													   pcmd->VtxOffset + global_vtx_offset,
													   0,
													   (UINT)cmd_list->VtxBuffer.Size,
													   pcmd->IdxOffset + global_idx_offset,
													   pcmd->ElemCount / 3);
				}
			}
			global_idx_offset += cmd_list->IdxBuffer.Size;
			global_vtx_offset += cmd_list->VtxBuffer.Size;
		}
	}

	static void present_callback(IDirect3DDevice9* device, IDirect3DSwapChain9* swapChain)
	{
		static bool init_done = false;
		if (!init_done)
		{
			ImGui::CreateContext();

			ImGui::StyleColorsDark();

			win32_init(FindWindowA("grcWindow", nullptr));
			dx9_init(device);

			init_done = true;
		}

		dx9_new_frame();
		win32_new_frame();
		ImGui::NewFrame();

		if (gDrawCallback)
		{
			gDrawCallback();
		}

		ImGui::EndFrame();

		IDirect3DSurface9* backBuffer = nullptr;
		if (FAILED(swapChain->GetBackBuffer(0, D3DBACKBUFFER_TYPE_MONO, &backBuffer)))
		{
			SPDLOG_DEBUG("IDirect3DSwapChain9::GetBackBuffer failed");
			return;
		}

		IDirect3DSurface9* prevRenderTarget = nullptr;
		if (FAILED(device->GetRenderTarget(0, &prevRenderTarget)))
		{
			SPDLOG_DEBUG("IDirect3DDevice9::GetRenderTarget failed");
			return;
		}

		if (backBuffer != prevRenderTarget)
		{
			if (FAILED(device->SetRenderTarget(0, backBuffer)))
			{
				SPDLOG_DEBUG("IDirect3DDevice9::SetRenderTarget failed");
				return;
			}
		}

		if (device->BeginScene() >= 0)
		{
			ImGui::Render();
			dx9_render_draw_data(ImGui::GetDrawData());
			device->EndScene();
		}

		prevRenderTarget->Release();
		backBuffer->Release();
	}

	static void pre_reset_callback(IDirect3DDevice9*) { dx9_invalidate_device_objects(); }
	static void post_reset_callback(IDirect3DDevice9*) { dx9_create_device_objects(); }

	void init()
	{
		MH_CreateHook(hook::get_pattern("55 8B EC 83 E4 F8 83 EC 14 53 8B 5D 08"),
					  &WndProc_detour,
					  reinterpret_cast<void**>(&WndProc_orig));

		d3d9_hook::set_present_callback(&present_callback);
		d3d9_hook::set_pre_reset_callback(&pre_reset_callback);
		d3d9_hook::set_post_reset_callback(&post_reset_callback);
	}

	void shutdown()
	{
		dx9_invalidate_device_objects();
		ImGui::DestroyContext();
	}

	void set_callback(void (*cb)()) { gDrawCallback = cb; }
}
