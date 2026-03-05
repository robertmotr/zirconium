#pragma once

#include "pch.h"
#include "zirconium_math.h"

// player offsets (relative to PE module base unless noted) 
#define OFF_PLAYER_BASE_PTR         0x1D88290
#define OFF_PLAYER_HEALTH           0x1DC1568
#define OFF_PLAYER_HEALTH_MAX       (OFF_PLAYER_HEALTH + 0x4)
#define OFF_PLAYER_POS_X            0x23427C8   // absolute
#define OFF_PLAYER_POS_Y            0x23427CC   // absolute
#define OFF_PLAYER_POS_Z            (OFF_PLAYER_POS_Y + 0x4) // absolute
#define OFF_PLAYER_NAME             (0x2347BC8 + 0x10C) // absolute
#define OFF_PLAYER_VIEW_ANGLE_X     0x23453A0   // absolute
#define OFF_PLAYER_VIEW_ANGLE_Y     0x23453A4   // absolute
#define OFF_PLAYER_STRUCT_ALT       (0x02330324 + 0x34) // absolute

// player offsets from dereferenced playerBase
#define OFF_PB_GRAVITY              0x8C
#define OFF_PB_SPEED                0x94

// weapon/inventory (relative to PE module base unless noted)
#define OFF_PRIMARY_RESERVE_AMMO    0x1F42B90
#define OFF_SECONDARY_RESERVE_AMMO  0x02342B98  // absolute, inconsistent with primary, investigate
#define OFF_GRENADES                0x1F42BD0
#define OFF_CLAYMORES               0x1F42BD8
#define OFF_MONKEY_BOMBS            0x1F42BDC
#define OFF_MONEY                   0x1F47D68

// game/round (misc)
#define OFF_ZOMBIE_ROUND            0x1F3B710
#define OFF_ZOMBIES_SPAWNED         0x1F30388

// zm entity list
#define ADDR_ZM_ENTITY_LIST_BASE    0x021C5828  // absolute
#define ZM_ENTITY_STRIDE            0x31C       // = sizeof(Zombie)
#define ZM_ENTITY_LIST_SZ           24

// zm internal offsets
#define OFF_ZM_POS_X                0x18
#define OFF_ZM_POS_Y                0x1C
#define OFF_ZM_POS_Z                0x20
#define OFF_ZM_PATHFINDING          0x40
#define OFF_ZM_BONE_MATRIX          0x118
#define OFF_ZM_CURRENT_HEALTH       0x1A8       // 16 floats, 4x4
#define OFF_ZM_START_HEALTH         0x1AC

// view matrix
#define ADDR_VIEW_MATRIX            0x0103AD40  // 16 floats, 4x4

// game function addresses (absolute, from Plutonium T6 offset table)
#define ADDR_COM_GET_CLIENT_DOBJ            0x660CA0
#define ADDR_CG_DOBJ_GET_WORLD_BONE_MATRIX  0x4E5100
#define ADDR_DOBJ_GET_BONE_INDEX            0x701490
#define ADDR_SL_FIND_STRING                 0x5E3B30  // GetTagFromName
#define ADDR_CG_GET_ENTITY                  0x4F03A0

// cg/cgs global pointers
#define ADDR_CG                             0x1113F9C
#define ADDR_CGS                            0x1113F78

// misc
#define ZOMBIE_BOX_HEIGHT           80          // rough estimate, vibe based
#define MAX_BONES                   128         // upper bound for fixed-size arrays

namespace zirconium {

    // game function typedefs — all __cdecl per decompilation
    // signatures verified against IDA PDB export (t6zm.exe.h / ida_functions.txt)
    typedef char*   (__cdecl* fn_Com_GetClientDObj)(int entityIndex, int localClientNumber);
    typedef int     (__cdecl* fn_CG_DObjGetWorldBoneMatrix)(void* pose, void* dobj, int boneIndex, float* rotationMatrix, float* worldPos, bool useAbsOrigin);
    typedef int     (__cdecl* fn_DObjGetBoneIndex)(int dobj, int boneNameSLStringID, unsigned char* boneIdxOut, int modelFilter);
    typedef int     (__cdecl* fn_SL_FindString)(const char* boneName);
    typedef void*   (__cdecl* fn_CG_GetEntity)(int localClientNum, int entityIndex);

    // T6 zombie bone names and skeleton connections — defined in zirconium_game.cpp
    // sizes are derived automatically so you can freely add/remove bones
    extern const char* BONE_NAMES[];
    extern const int   BONE_NAME_COUNT;
    extern const int   BONE_CONNECTIONS[][2];
    extern const int   BONE_CONNECTION_COUNT;

    typedef enum _BoneType
    {
        BONE_UNKNOWN            = -1,
		BONE_ROOT,
		BONE_SPINE1,
		BONE_SPINE2,
		BONE_SPINE3,
		BONE_NECK,
		BONE_HEAD,
		BONE_RIGHT_CLAVICLE,
		BONE_RIGHT_SHOULDER,
		BONE_RIGHT_ELBOW,
		BONE_RIGHT_HAND,
		BONE_LEFT_CLAVICLE,
		BONE_LEFT_SHOULDER,
		BONE_LEFT_ELBOW,
		BONE_LEFT_HAND,
		BONE_RIGHT_THIGH,
		BONE_LEFT_THIGH,
        BONE_MAX                 = (INT32_MAX)
    } BoneType;

    inline const char* boneTypeToString(BoneType bone) {
        static const char* names[] = {
            "BONE_ROOT",
            "BONE_SPINE1", "BONE_SPINE2", "BONE_SPINE3",
            "BONE_NECK", "BONE_HEAD",
            "BONE_RIGHT_CLAVICLE", "BONE_RIGHT_SHOULDER", "BONE_RIGHT_ELBOW", "BONE_RIGHT_HAND",
            "BONE_LEFT_CLAVICLE", "BONE_LEFT_SHOULDER", "BONE_LEFT_ELBOW", "BONE_LEFT_HAND",
            "BONE_RIGHT_THIGH", "BONE_LEFT_THIGH"
        };
        if (bone < 0 || bone >= static_cast<int>(sizeof(names) / sizeof(names[0])))
            return "BONE_UNKNOWN";
        return names[bone];
    }

#pragma pack(push, 1)
    typedef struct _Player
    {
        // FIXME
        zirconium::matrix<4, 4, float>  viewMatrix;
		zirconium::vector<3, int>       playerPosition;
		zirconium::vector<2, float>     playerAngle;
    } Player;
#pragma pack(pop)

#pragma pack(push, 1)
    typedef struct _Zombie
    {
        uint8_t                         _pad0[OFF_ZM_POS_X];                                        // 0x000 -> 0x018
        zirconium::vector<3, float>     position;                                                   // 0x018 -> 0x024
        uint8_t                         _pad1[OFF_ZM_PATHFINDING - (OFF_ZM_POS_X + sizeof(zirconium::vector<3, float>))]; // 0x024 -> 0x040
        float                           pathfindingDir;                                             // 0x040 -> 0x044
        uint8_t                         _pad2[OFF_ZM_BONE_MATRIX - (OFF_ZM_PATHFINDING + sizeof(float))]; // 0x044 -> 0x118
        zirconium::matrix<4, 4, float>  boneMatrix;                                                 // 0x118 -> 0x158
        uint8_t                         _pad3[OFF_ZM_CURRENT_HEALTH - (OFF_ZM_BONE_MATRIX + sizeof(zirconium::matrix<4, 4, float>))]; // 0x158 -> 0x1A8
        unsigned int                    currentHealth;                                              // 0x1A8 -> 0x1AC
        unsigned int                    startHealth;                                                // 0x1AC -> 0x1B0
        uint8_t                         _pad4[ZM_ENTITY_STRIDE - (OFF_ZM_START_HEALTH + sizeof(unsigned int))]; // 0x1B0 -> 0x31C

        bool isAlive() const { return currentHealth > 0; }
    } Zombie;
#pragma pack(pop)

    static_assert(sizeof(Zombie) == ZM_ENTITY_STRIDE,                       "Zombie size must be 0x31C");
    static_assert(offsetof(Zombie, position) == OFF_ZM_POS_X,               "position offset mismatch");
    static_assert(offsetof(Zombie, pathfindingDir) == OFF_ZM_PATHFINDING,   "pathfinding offset mismatch");
    static_assert(offsetof(Zombie, boneMatrix) == OFF_ZM_BONE_MATRIX,       "boneMatrix offset mismatch");
    static_assert(offsetof(Zombie, currentHealth) == OFF_ZM_CURRENT_HEALTH, "currentHealth offset mismatch");
    static_assert(offsetof(Zombie, startHealth) == OFF_ZM_START_HEALTH,     "startHealth offset mismatch");

#pragma pack(push, 1)
    typedef struct _Game
    {
        // FIXME
    } Game;
#pragma pack(pop)

#pragma pack(push, 1)
    typedef struct _World
    {
        // FIXME
    } World;
#pragma pack(pop)

    typedef struct _GameContext
    {
        Player player;
        Zombie zmEntityList[ZM_ENTITY_LIST_SZ];
        World  world;
        bool game_initGameContext();
    } GameContext;

    // Address book
    // Initialized once at startup; provides typed pointers to game memory
    // for use by aimbot, ESP, memory table, etc.
    struct AddressBook {
        uintptr_t       moduleBase          = 0;
        uintptr_t       playerBase          = 0;

        // player
        unsigned int*   playerHealth        = nullptr;
        unsigned int*   playerHealthMax     = nullptr;
        float*          playerPosX          = nullptr;
        float*          playerPosY          = nullptr;
        float*          playerPosZ          = nullptr;
        unsigned int*   playerGravity       = nullptr;
        unsigned int*   playerSpeed         = nullptr;
        char*           playerName          = nullptr;
        float*          viewAngleX          = nullptr;
        float*          viewAngleY          = nullptr;

        // weapons/inventory
        unsigned int*   primaryReserveAmmo  = nullptr;
        unsigned int*   secondaryReserveAmmo = nullptr;
        unsigned int*   grenades            = nullptr;
        unsigned int*   claymores           = nullptr;
        unsigned int*   monkeyBombs         = nullptr;
        unsigned int*   money               = nullptr;

        // game state
        unsigned int*   zombieRound         = nullptr;
        unsigned int*   zombiesSpawned      = nullptr;

        // zombie entity list (cast directly to padded Zombie struct)
        Zombie*         zmEntityList        = nullptr;

        // view/projection matrix (16 contiguous floats)
        float*          viewMatrix          = nullptr;

        // game functions
        fn_Com_GetClientDObj         Com_GetClientDObj        = nullptr;
        fn_CG_DObjGetWorldBoneMatrix CG_DObjGetWorldBoneMatrix = nullptr;
        fn_DObjGetBoneIndex          DObjGetBoneIndex         = nullptr;
        fn_SL_FindString             SL_FindString            = nullptr;
        fn_CG_GetEntity              CG_GetEntity             = nullptr;

        // cached bone string IDs (resolved once via SL_FindString)
        int  boneStringIDs[MAX_BONES] = {};
        bool bonesResolved             = false;

        // entity index offset for zombie-to-centity mapping (configurable via debug tab)
        int  zombieEntityOffset        = 0;

        bool resolved = false;

        bool resolve();
        bool resolveBoneIDs();
    };

    extern AddressBook g_addrs;

    void __stdcall game_drawESP(void);
    void __stdcall game_drawBoxESP(void);
    void __stdcall game_drawSkeletonESP(void);

} // namespace zirconium
