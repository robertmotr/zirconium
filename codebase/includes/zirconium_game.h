#pragma once

#include "pch.h"
#include "zirconium_math.h"
#include "t6zm.exe.h"

/* TODO: a note to future self
the game (at time of writing) does not have ASLR enabled, nor do most values seem to be allocated
on the stack or the heap. almost all of these addresses live in .text/.data/.rdata
which makes things nice for us. if plutonium devs ever decide to build with ASLR (unlikely, they'd need the full game source)
then this is all fucked and we have to determine all of these addresses by pointer chains, and resolving module addresses  

just in case this mysteriously breaks later...
*/

// weapon/inventory
#define ADDR_PRIMARY_MAIN_AMMO                 0x2342BD4
#define ADDR_PRIMARY_RESERVE_AMMO              0x2342B98
#define ADDR_SECONDARY_MAIN_AMMO               0x2342BCC
#define ADDR_SECONDARY_RESERVE_AMMO            0x2342B90
#define ADDR_GRENADES                          0x2342BD0
#define ADDR_CLAYMORES                         0x2342BD8
#define ADDR_MONKEY_BOMBS                      0x2342BDC
#define ADDR_MONEY                             0x2347D68

// game/round (misc)
#define ADDR_ZOMBIE_ROUND                      0x1F3B710
#define ADDR_ZOMBIES_SPAWNED                   0x1F30388

// gentity/zm entity array
#define ADDR_GENTITY_ARRAY                     0x021C13C0
#define ZM_ENTITY_LIST_SZ                      24

// view matrix/view angles
#define ADDR_VIEW_MATRIX                       0x0103AD40
#define ADDR_PLAYER_VIEW_ANGLE_X               0x23453A0   
#define ADDR_PLAYER_VIEW_ANGLE_Y               0x23453A4

// game function addresses
#define ADDR_FN_COM_GET_CLIENT_DOBJ            0x660CA0
#define ADDR_FN_CG_DOBJ_GET_WORLD_BONE_MATRIX  0x4E5100
#define ADDR_FN_DOBJ_GET_BONE_INDEX            0x701490
#define ADDR_FN_SL_FIND_STRING                 0x5E3B30  // aka GetTagFromName(maybe?)
#define ADDR_FN_CG_GET_ENTITY                  0x4F03A0
#define ADDR_FN_CG_GET_ENTITY_BONE_INFO        0x4E6260
#define ADDR_FN_SL_CONVERT_TO_STRING           0x835C00
#define ADDR_FN_DOBJ_GET_BONE_NAME             0x532230

// hook-target functions (detoured by the feature checkboxes)
#define ADDR_FN_NOSPREAD                       0x42E950
#define ADDR_FN_NORECOIL                       0x64BBE0
#define ADDR_FN_GODMODE                        0x829790

// cg/cgs global pointers
#define ADDR_CLIENT_GAME_STRUCT                0x1113F9C
#define ADDR_CLIENT_GAME_STATIC_STRUCT         0x1113F78
#define ADDR_CLIENT_ACTIVE_STRUCT              0x119CB84

// dvars
#define ADDR_DVAR_GRAVITY                      0x31FC4D8
#define ADDR_DVAR_JUMP_HEIGHT                  0x31FC418
#define ADDR_DVAR_SPEED                        0x2338668

namespace zirconium {

    constexpr uint32_t MAX_CLIENT_ENTITIES  = 1024;
    constexpr uint32_t ZM_ENTITY_FIRST_SLOT = 21;
    constexpr int      MAX_REASONABLE_BONES = 256;

    constexpr unsigned int EF_DEAD_THRESHOLD = 0x00040802;

    // left side = right side - 1 for all limb bones
    enum BoneIndex : int {
        // Core spine
        BONE_PELVIS             =  1,
        BONE_SPINE_LUMBAR       =  5,
        BONE_SPINE_ABDOMEN      = 10,
        BONE_SPINE_THORACIC     = 73,
        BONE_SPINE_CERVICAL     = 74,  // neck-spine joint
        BONE_HEAD_BASE          = 75,
        BONE_HEAD_FOREHEAD      = 88,
        BONE_HEAD_TOP           = 97,

        // Right side
        BONE_HIP_RIGHT          =  4,
        BONE_THIGH_RIGHT        =  7,
        BONE_KNEE_RIGHT         = 14,
        BONE_ANKLE_RIGHT        = 12,
        BONE_FOOT_RIGHT         = 17,
        BONE_CLAVICLE_RIGHT     = 22,
        BONE_SHOULDER_RIGHT     = 28,
        BONE_ELBOW_RIGHT        = 30,
        BONE_WRIST_RIGHT        = 40,
        BONE_PALM_BRIDGE_RIGHT  = 38,  // wrist/palm interconnect
        BONE_PALM_BASE_RIGHT    = 50,

        // Left side (right - 1)
        BONE_HIP_LEFT           =  3,
        BONE_THIGH_LEFT         =  6,
        BONE_KNEE_LEFT          = 13,
        BONE_ANKLE_LEFT         = 11,
        BONE_FOOT_LEFT          = 16,
        BONE_CLAVICLE_LEFT      = 21,
        BONE_SHOULDER_LEFT      = 27,
        BONE_ELBOW_LEFT         = 29,
        BONE_WRIST_LEFT         = 39,
        BONE_PALM_BRIDGE_LEFT   = 37,  // wrist/palm interconnect
        BONE_PALM_BASE_LEFT     = 49,

        // Right hand fingers (knuckle / middle / tip)
        BONE_R_THUMB_KNUCKLE    = 62,  BONE_R_THUMB_TIP        = 72,
        BONE_R_INDEX_KNUCKLE    = 42,  BONE_R_INDEX_MIDDLE     = 54,  BONE_R_INDEX_TIP    = 64,
        BONE_R_MIDDLE_KNUCKLE   = 44,  BONE_R_MIDDLE_MIDDLE    = 56,  BONE_R_MIDDLE_TIP   = 66,
        BONE_R_RING_KNUCKLE     = 48,  BONE_R_RING_MIDDLE      = 60,  BONE_R_RING_TIP     = 70,
        BONE_R_PINKY_KNUCKLE    = 46,  BONE_R_PINKY_MIDDLE     = 58,  BONE_R_PINKY_TIP    = 68,

        // Left hand fingers (right - 1)
        BONE_L_THUMB_KNUCKLE    = 61,  BONE_L_THUMB_TIP        = 71,
        BONE_L_INDEX_KNUCKLE    = 41,  BONE_L_INDEX_MIDDLE     = 53,  BONE_L_INDEX_TIP    = 63,
        BONE_L_MIDDLE_KNUCKLE   = 43,  BONE_L_MIDDLE_MIDDLE    = 55,  BONE_L_MIDDLE_TIP   = 65,
        BONE_L_RING_KNUCKLE     = 47,  BONE_L_RING_MIDDLE      = 59,  BONE_L_RING_TIP     = 69,
        BONE_L_PINKY_KNUCKLE    = 45,  BONE_L_PINKY_MIDDLE     = 57,  BONE_L_PINKY_TIP    = 67,
    };

    struct BoneLink {
        BoneIndex from;
        BoneIndex to;
    };

    constexpr BoneLink BONE_LINKS[] = {
        // spine: pelvis -> lumbar -> abdomen -> thoracic -> cervical -> head_base
        { BONE_PELVIS,           BONE_SPINE_LUMBAR },
        { BONE_SPINE_LUMBAR,     BONE_SPINE_ABDOMEN },
        { BONE_SPINE_ABDOMEN,    BONE_SPINE_THORACIC },
        { BONE_SPINE_THORACIC,   BONE_SPINE_CERVICAL },
        { BONE_SPINE_CERVICAL,   BONE_HEAD_BASE },

        // hips
        { BONE_PELVIS,           BONE_HIP_LEFT },
        { BONE_PELVIS,           BONE_HIP_RIGHT },

        // left leg
        { BONE_HIP_LEFT,         BONE_THIGH_LEFT },
        { BONE_THIGH_LEFT,       BONE_KNEE_LEFT },
        { BONE_KNEE_LEFT,        BONE_ANKLE_LEFT },
        { BONE_ANKLE_LEFT,       BONE_FOOT_LEFT },

        // right leg
        { BONE_HIP_RIGHT,        BONE_THIGH_RIGHT },
        { BONE_THIGH_RIGHT,      BONE_KNEE_RIGHT },
        { BONE_KNEE_RIGHT,       BONE_ANKLE_RIGHT },
        { BONE_ANKLE_RIGHT,      BONE_FOOT_RIGHT },

        // clavicles (horizontal bar across chest, anchored at cervical midpoint)
        { BONE_SPINE_CERVICAL,   BONE_CLAVICLE_LEFT },
        { BONE_SPINE_CERVICAL,   BONE_CLAVICLE_RIGHT },

        // left arm
        { BONE_CLAVICLE_LEFT,    BONE_SHOULDER_LEFT },
        { BONE_SHOULDER_LEFT,    BONE_ELBOW_LEFT },
        { BONE_ELBOW_LEFT,       BONE_WRIST_LEFT },

        // right arm
        { BONE_CLAVICLE_RIGHT,   BONE_SHOULDER_RIGHT },
        { BONE_SHOULDER_RIGHT,   BONE_ELBOW_RIGHT },
        { BONE_ELBOW_RIGHT,      BONE_WRIST_RIGHT },

        // wrist -> palm interconnect -> palm base
        { BONE_WRIST_RIGHT,      BONE_PALM_BRIDGE_RIGHT },
        { BONE_PALM_BRIDGE_RIGHT, BONE_PALM_BASE_RIGHT },
        { BONE_WRIST_LEFT,       BONE_PALM_BRIDGE_LEFT },
        { BONE_PALM_BRIDGE_LEFT, BONE_PALM_BASE_LEFT },

        // ---- Finger bones ----
        // right hand
        { BONE_PALM_BASE_RIGHT,  BONE_R_INDEX_KNUCKLE },
        { BONE_R_INDEX_KNUCKLE,  BONE_R_INDEX_MIDDLE },
        { BONE_R_INDEX_MIDDLE,   BONE_R_INDEX_TIP },
        { BONE_PALM_BASE_RIGHT,  BONE_R_MIDDLE_KNUCKLE },
        { BONE_R_MIDDLE_KNUCKLE, BONE_R_MIDDLE_MIDDLE },
        { BONE_R_MIDDLE_MIDDLE,  BONE_R_MIDDLE_TIP },
        { BONE_PALM_BASE_RIGHT,  BONE_R_RING_KNUCKLE },
        { BONE_R_RING_KNUCKLE,   BONE_R_RING_MIDDLE },
        { BONE_R_RING_MIDDLE,    BONE_R_RING_TIP },
        { BONE_PALM_BASE_RIGHT,  BONE_R_PINKY_KNUCKLE },
        { BONE_R_PINKY_KNUCKLE,  BONE_R_PINKY_MIDDLE },
        { BONE_R_PINKY_MIDDLE,   BONE_R_PINKY_TIP },
        { BONE_PALM_BASE_RIGHT,  BONE_R_THUMB_KNUCKLE },
        { BONE_R_THUMB_KNUCKLE,  BONE_R_THUMB_TIP },

        // left hand
        { BONE_PALM_BASE_LEFT,   BONE_L_INDEX_KNUCKLE },
        { BONE_L_INDEX_KNUCKLE,  BONE_L_INDEX_MIDDLE },
        { BONE_L_INDEX_MIDDLE,   BONE_L_INDEX_TIP },
        { BONE_PALM_BASE_LEFT,   BONE_L_MIDDLE_KNUCKLE },
        { BONE_L_MIDDLE_KNUCKLE, BONE_L_MIDDLE_MIDDLE },
        { BONE_L_MIDDLE_MIDDLE,  BONE_L_MIDDLE_TIP },
        { BONE_PALM_BASE_LEFT,   BONE_L_RING_KNUCKLE },
        { BONE_L_RING_KNUCKLE,   BONE_L_RING_MIDDLE },
        { BONE_L_RING_MIDDLE,    BONE_L_RING_TIP },
        { BONE_PALM_BASE_LEFT,   BONE_L_PINKY_KNUCKLE },
        { BONE_L_PINKY_KNUCKLE,  BONE_L_PINKY_MIDDLE },
        { BONE_L_PINKY_MIDDLE,   BONE_L_PINKY_TIP },
        { BONE_PALM_BASE_LEFT,   BONE_L_THUMB_KNUCKLE },
        { BONE_L_THUMB_KNUCKLE,  BONE_L_THUMB_TIP },
    };
    constexpr int BONE_LINK_COUNT = sizeof(BONE_LINKS) / sizeof(BONE_LINKS[0]);

    // aimbot bone target options
    struct BoneOption {
        BoneIndex   index;
        const char* name;
    };

    constexpr BoneOption AIMBOT_BONE_OPTIONS[] = {
        // head / neck
        { BONE_HEAD_TOP,            "Head (top)" },
        { BONE_HEAD_FOREHEAD,       "Head (forehead)" },
        { BONE_HEAD_BASE,           "Neck" },

        // spine
        { BONE_SPINE_CERVICAL,      "Spine (cervical)" },
        { BONE_SPINE_THORACIC,      "Upper chest" },
        { BONE_SPINE_ABDOMEN,       "Abdomen" },
        { BONE_SPINE_LUMBAR,        "Lower torso" },
        { BONE_PELVIS,              "Pelvis" },

        // right leg
        { BONE_HIP_RIGHT,           "Hip (R)" },
        { BONE_THIGH_RIGHT,         "Thigh (R)" },
        { BONE_KNEE_RIGHT,          "Knee (R)" },
        { BONE_ANKLE_RIGHT,         "Ankle (R)" },
        { BONE_FOOT_RIGHT,          "Foot (R)" },

        // left leg
        { BONE_HIP_LEFT,            "Hip (L)" },
        { BONE_THIGH_LEFT,          "Thigh (L)" },
        { BONE_KNEE_LEFT,           "Knee (L)" },
        { BONE_ANKLE_LEFT,          "Ankle (L)" },
        { BONE_FOOT_LEFT,           "Foot (L)" },

        // right arm
        { BONE_CLAVICLE_RIGHT,      "Clavicle (R)" },
        { BONE_SHOULDER_RIGHT,      "Shoulder (R)" },
        { BONE_ELBOW_RIGHT,         "Elbow (R)" },
        { BONE_WRIST_RIGHT,         "Wrist (R)" },
        { BONE_PALM_BRIDGE_RIGHT,   "Palm bridge (R)" },
        { BONE_PALM_BASE_RIGHT,     "Palm base (R)" },

        // left arm
        { BONE_CLAVICLE_LEFT,       "Clavicle (L)" },
        { BONE_SHOULDER_LEFT,       "Shoulder (L)" },
        { BONE_ELBOW_LEFT,          "Elbow (L)" },
        { BONE_WRIST_LEFT,          "Wrist (L)" },
        { BONE_PALM_BRIDGE_LEFT,    "Palm bridge (L)" },
        { BONE_PALM_BASE_LEFT,      "Palm base (L)" },

        // right hand fingers
        { BONE_R_THUMB_KNUCKLE,     "Thumb knuckle (R)" },
        { BONE_R_THUMB_TIP,         "Thumb tip (R)" },
        { BONE_R_INDEX_KNUCKLE,     "Index knuckle (R)" },
        { BONE_R_INDEX_MIDDLE,      "Index middle (R)" },
        { BONE_R_INDEX_TIP,         "Index tip (R)" },
        { BONE_R_MIDDLE_KNUCKLE,    "Middle knuckle (R)" },
        { BONE_R_MIDDLE_MIDDLE,     "Middle middle (R)" },
        { BONE_R_MIDDLE_TIP,        "Middle tip (R)" },
        { BONE_R_RING_KNUCKLE,      "Ring knuckle (R)" },
        { BONE_R_RING_MIDDLE,       "Ring middle (R)" },
        { BONE_R_RING_TIP,          "Ring tip (R)" },
        { BONE_R_PINKY_KNUCKLE,     "Pinky knuckle (R)" },
        { BONE_R_PINKY_MIDDLE,      "Pinky middle (R)" },
        { BONE_R_PINKY_TIP,         "Pinky tip (R)" },

        // left hand fingers
        { BONE_L_THUMB_KNUCKLE,     "Thumb knuckle (L)" },
        { BONE_L_THUMB_TIP,         "Thumb tip (L)" },
        { BONE_L_INDEX_KNUCKLE,     "Index knuckle (L)" },
        { BONE_L_INDEX_MIDDLE,      "Index middle (L)" },
        { BONE_L_INDEX_TIP,         "Index tip (L)" },
        { BONE_L_MIDDLE_KNUCKLE,    "Middle knuckle (L)" },
        { BONE_L_MIDDLE_MIDDLE,     "Middle middle (L)" },
        { BONE_L_MIDDLE_TIP,        "Middle tip (L)" },
        { BONE_L_RING_KNUCKLE,      "Ring knuckle (L)" },
        { BONE_L_RING_MIDDLE,       "Ring middle (L)" },
        { BONE_L_RING_TIP,          "Ring tip (L)" },
        { BONE_L_PINKY_KNUCKLE,     "Pinky knuckle (L)" },
        { BONE_L_PINKY_MIDDLE,      "Pinky middle (L)" },
        { BONE_L_PINKY_TIP,         "Pinky tip (L)" },
    };
    constexpr int AIMBOT_BONE_OPTION_COUNT = sizeof(AIMBOT_BONE_OPTIONS) / sizeof(AIMBOT_BONE_OPTIONS[0]);

    enum AimbotTargetMode : int 
    {
        AIMBOT_TARGET_MODE_NONE,
        AIMBOT_TARGET_MODE_NEAREST,
        AIMBOT_TARGET_MODE_CROSSHAIR,
        AIMBOT_TARGET_MODE_LOWEST_HP,
        AIMBOT_TARGET_MODE_HIGHEST_HP,
        AIMBOT_TARGET_MODE_MAX,
    };

    typedef DObj*       (__cdecl* Com_GetClientDObj)(int entityIndex, int localClientNumber);
    typedef int         (__cdecl* CG_DObjGetWorldBoneMatrix)(const cpose_t* pose, DObj* dobj, int boneIndex, vec3_t *tagMat, vec3_t *origin, bool useWiiUAimPt);
    typedef int         (__cdecl* DObjGetBoneIndex)(const DObj *dobj, int boneNameSLStringID, unsigned char* boneIdxOut, int modelFilter);
    typedef int         (__cdecl* SL_FindString)(const char* boneName);
    typedef const char* (__cdecl* SL_ConvertToString)(unsigned int stringValue);
    typedef centity_t*  (__cdecl* CG_GetEntity)(int localClientNum, int entityIndex);
    typedef cg_t*       (__cdecl* CG_GetLocalClientGlobals)(int localClientNum);
    typedef int         (__cdecl* CG_GetEntityBoneInfo)(int entID, int boneIndex, vec3_t *bonePos, vec3_t *boneAxis, const char **boneName);
    typedef char*       (__cdecl* DObjGetBoneName)(const DObj *dobj, int boneIndex);

    struct AddressBook {
        // game functions
        Com_GetClientDObj         Com_GetClientDObj         = nullptr;
        CG_DObjGetWorldBoneMatrix CG_DObjGetWorldBoneMatrix = nullptr;
        DObjGetBoneIndex          DObjGetBoneIndex          = nullptr;
        SL_FindString             SL_FindString             = nullptr;
        CG_GetEntity              CG_GetEntity              = nullptr;
        CG_GetEntityBoneInfo      CG_GetEntityBoneInfo      = nullptr;
        SL_ConvertToString        SL_ConvertToString        = nullptr;
        DObjGetBoneName           DObjGetBoneName           = nullptr;

        // weapons/inventory/misc values useful to reference quickly
        unsigned int*   primaryMainAmmo      = nullptr;
        unsigned int*   primaryReserveAmmo   = nullptr;
        unsigned int*   secondaryMainAmmo    = nullptr;
        unsigned int*   secondaryReserveAmmo = nullptr;
        unsigned int*   grenades             = nullptr;
        unsigned int*   claymores            = nullptr;
        unsigned int*   monkeyBombs          = nullptr;
        unsigned int*   money                = nullptr;
        unsigned int*   zombieRound          = nullptr;
        unsigned int*   zombiesSpawned       = nullptr;

        // view/projection matrix
        float*          viewMatrix           = nullptr;

        // helpful global structs
        gentity_t      *gentityPlayer        = nullptr;
        centity_t      *centityPlayer        = nullptr;
        cg_t           *clientGame           = nullptr;
        cgs_t          *clientGameStatic     = nullptr;
        clientActive_t *clientActive         = nullptr;

        // dvars
        dvar_t         *dvarGravity         = nullptr;
        dvar_t         *dvarSpeed           = nullptr;
        dvar_t         *dvarJumpHeight      = nullptr;

        bool resolved = false;

        bool resolveStatic();
        bool refreshVolatile();
    };

    extern AddressBook g_addrBook;

    void __stdcall game_drawESP(void);
    void __stdcall game_aimbot(void);

    int    __cdecl godModeHook(unsigned int a1);
    float* __cdecl noRecoilHook(int a1, float* a2, float* a3, DWORD* a4, char a5, float* a6);
    int    __cdecl noSpreadHook(int a1, int a2, float* x, float* y);

} // namespace zirconium
