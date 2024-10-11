#pragma once

#include "pch.h"

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
}