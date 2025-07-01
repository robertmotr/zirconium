#include "globals.h" 

namespace gameVars {
	std::vector<std::string> bones = {
		"BONE_ROOT",
		"BONE_SPINE1",
		"BONE_SPINE2",
		"BONE_SPINE3",
		"BONE_NECK",
		"BONE_HEAD",
		"BONE_RIGHT_CLAVICLE",
		"BONE_RIGHT_SHOULDER",
		"BONE_RIGHT_ELBOW",
		"BONE_RIGHT_HAND",
		"BONE_LEFT_CLAVICLE",
		"BONE_LEFT_SHOULDER",
		"BONE_LEFT_ELBOW",
		"BONE_LEFT_HAND",
		"BONE_RIGHT_THIGH",
		"BONE_LEFT_THIGH"
	};
}

// renderVars are global vars used for setting up imgui/imgui backend
namespace renderVars {
	ID3D11RenderTargetView*			renderTargetView = nullptr; // used for rendering ImGui on top of the game
	bool							initialized = false; 		// check if ImGui is initialized
	ImGuiIO*						io = nullptr; 				// stored globally to reduce overhead inside renderOverlay
	ImGuiContext*					ctx = nullptr; 				// stored globally to reduce overhead inside renderOverlay
	HWND							g_hwnd = nullptr; 			// background window we're running on top of
	RECT							rect = {}; 					// used for dynamically checking if wnd was resized
}

// guiVars are globals used often for showing content and/or menu logic
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
	int								aim_preset = 0;
	int								aim_fov = 0;
	float							lock_speed;
}

namespace hookVars {
	BYTE*							resumeAddr = nullptr;
	BYTE*							oPresent = nullptr;
	ID3D11Device*					device = nullptr;
	ID3D11DeviceContext*			deviceContext = nullptr;
}
