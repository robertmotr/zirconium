#pragma once
#include "pch.h"

#define JMP_SZ              5 					// jmp size
#define JMP                 0xE9 				// jmp opcode
#define NOP                 0x90 				// nop opcode
#define TRAMPOLINE_SZ       6 					// size of the trampoline 
#define HOOK_OFFSET  	    0x1F 				// offset from hookVars::oPresent. This is where we hook our initial jmp
#define PRESENT_INDEX       8 					// index 8 for Present in IDXGISwapChain vmt

#define PE_MODULE_NAME      "plutonium-bootstrapper-win32.exe" 	// name of the module to hook
#define WINDOW_NAME         "Plutonium T6 Zombies (r4839)" 	// name of the window to hook

using MEM_TYPES = std::variant<unsigned int, float, std::string>;

#ifdef _MENU_ONLY
// Placeholder data for testing without actual game loaded
struct PlaceholderData {
    unsigned int health;
    unsigned int healthMax;
    float xCoordinate;
    float yCoordinate;
    float zCoordinate;
    unsigned int gravity;
    unsigned int staticSpeed;
    char localName[256];
    unsigned int crosshair;
    float viewAngleX;
    float viewAngleY;
    unsigned int primaryWeaponAmmo;
    unsigned int secondaryWeaponAmmo;
    unsigned int grenades;
    unsigned int claymores;
    unsigned int monkeyBombs;
    unsigned int money;
    unsigned int zombieRoundDisplay;
    unsigned int zombiesSpawnedIn;
    char zmEntityList0[256];
    float zmEntityPathfindingDirection;
    float zmEntityCoordinateX;
    float zmEntityCoordinateY;
    float zmEntityCoordinateZ;
    unsigned int zmEntityStartHealth;
    unsigned int zmEntityCurrentHealth;
    float boneMatrix[4][4];
    char playerStructWithOffset[256];
    float viewMatrix[16];
};
#endif

struct memoryTableEntry {
	std::string name;
	uintptr_t addr;
    MEM_TYPES value;
	std::string description;
	std::string structName;
};

namespace gameVars {
    extern std::vector<std::string> bones;
};

// for ImGui setup
namespace renderVars {
	extern ID3D11RenderTargetView* renderTargetView;
	extern bool initialized; // check if ImGui is initialized
	extern ImGuiIO* io; // stored globally to reduce overhead inside renderOverlay
	extern ImGuiContext* ctx; // same as above
	extern HWND g_hwnd;
	extern RECT rect; // for smart menu resizing, also to reduce overhead
	extern WNDPROC oWndProc; // original window procedure, saved before hooking
}

// for actual ImGui content
namespace guiVars {
    extern std::vector<memoryTableEntry> 		memoryTable;
    extern bool 								showMenu;
    extern ImGuiTabBarFlags 					tab_bar_flags;
    extern unsigned int 						memory_table_idx;
    extern unsigned int							struct_view_idx;
    extern bool 								aimbot_checkbox;
    extern int 									aimbot_radio_btn_sel;
    extern bool 								lock_until_eliminated;
    extern int 									aim_preset;
    extern int 									aim_fov;
    extern float 								lock_speed;
}

namespace hookVars {
	extern BYTE* resumeAddr;
    extern BYTE* oPresent;
	extern ID3D11Device* device;
	extern ID3D11DeviceContext* deviceContext;
}
