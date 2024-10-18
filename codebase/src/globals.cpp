#include "globals.h" 
#include "scan_mem.h"

// renderVars are global vars used for setting up imgui or imgui backend stuff
namespace renderVars {
	bool                            initialized = false; // check if ImGui is initialized
	ImGuiIO*						io = nullptr; // stored globally to reduce overhead inside renderOverlay
	ImGuiContext*					ctx = nullptr; // stored globally to reduce overhead inside renderOverlay
	LPDIRECT3DDEVICE9               g_pDevice = nullptr;
	HWND                            g_hwnd = nullptr; // background window we're running on top of
	RECT                            rect = {}; // used for dynamically checking if wnd was resized
	D3DPRESENT_PARAMETERS*			g_pD3DPP = nullptr;
}

// guiVars on the other hand are globals we'll be using often for showing content and/or menu logic
namespace guiVars {
	bool                            showMenu = true;
	ImGuiTabBarFlags                tab_bar_flags = ImGuiTabBarFlags_Reorderable
															| ImGuiTabBarFlags_NoCloseWithMiddleMouseButton
															| ImGuiTabBarFlags_FittingPolicyScroll;

	// TODO: double check offset correctness
	std::vector<memoryTableEntry>	memoryTable = {};

	unsigned int                    memory_table_idx = 0;
	unsigned int                    struct_view_idx = 0;

	bool							aimbot_checkbox = false;
	int								aimbot_radio_btn_sel = 0;
	bool							select_aim_key = false;
	bool							lock_until_eliminated = false;
}

namespace hookVars {
	FARPROC							oEndScene = nullptr; // original EndScene function address
	volatile LPDIRECT3DDEVICE9		pDevice = nullptr; // IDirect3DDevice9 pointer being used in the target application
	BYTE							newEndSceneAsm[OENDSCENE_NUM_BYTES_OVERWRITTEN] = {0};
	BYTE							oldEndSceneAsm[OENDSCENE_NUM_BYTES_OVERWRITTEN] = {0};
	BYTE							expectedEndSceneAsm[OENDSCENE_NUM_BYTES_OVERWRITTEN] = {0x6A, 0x14, // push 14h
																								0xB8, 0x70, 0x47, 0x08, 0x10 }; // mov eax, offset loc_10084770
	DWORD								relJmpAddrToHook = NULL;
}