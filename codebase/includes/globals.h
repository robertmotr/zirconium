#pragma once
#include "pch.h"

#define PE_MODULE_NAME      "plutonium-bootstrapper-win32.exe"
#define WINDOW_NAME         "Plutonium T6 Zombies (r4060)"
#define ENDSCENE_INDEX      42 // index of EndScene() in IDirect3DDevice9 vtable
#define TRAMPOLINE_SZ       7 // # bytes overwritten in oEndScene in order to jmp to our hook (aka trampoline)
#define JMP_SZ              5
#define JMP_MODRM_SZ        6
#define JMP                 0xE9 // jmp opcode
#define CALL                0xE8 // call opcode
#define PUSH                0x68 // push opcode
#define JMP_MODRM           0xFF // jmp modrm opcode
#define JMP_SHORT           0xEB // jmp short opcode
#define MODRM_DISP32        0x25 // modrm disp32 opcode
#define NOP                 0x90 // nop opcode

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
	extern bool initialized; // check if ImGui is initialized
	extern ImGuiIO* io; // stored globally to reduce overhead inside renderOverlay
	extern ImGuiContext* ctx; // same as above
	extern LPDIRECT3DDEVICE9 g_pDevice;
	extern HWND g_hwnd;
	extern RECT rect; // for smart menu resizing, also to reduce overhead
	extern D3DPRESENT_PARAMETERS* g_pD3DPP;
}

// for actual ImGui content
namespace guiVars {
    extern std::vector<memoryTableEntry> memoryTable;
	extern bool showMenu;
	extern ImGuiTabBarFlags tab_bar_flags;
	extern unsigned int memory_table_idx;
	extern unsigned int	struct_view_idx;
    extern bool aimbot_checkbox;
    extern int aimbot_radio_btn_sel;
    extern bool lock_until_eliminated;
    extern int aim_preset;
    extern int aim_fov;
    extern float lock_speed;
}

namespace hookVars {
    extern BYTE* oEndScene;
	extern volatile LPDIRECT3DDEVICE9 pDevice; // IDirect3DDevice9 pointer being used in the target application
    extern BYTE* trampoline;
}