#include "pch.h"

#include "zirconium_game.h"
#include "zirconium_globals.h"
#include "zirconium_hook.h"     // PE_MODULE_NAME
#include "zirconium_memory.h"

zirconium::AddressBook      zirconium::g_addrs{};

const char* zirconium::BONE_NAMES[] = {
    "j_ankle_le", "j_ankle_ri", "j_ball_le", "j_ball_ri", "j_barrel",
    "j_clavicle_le", "j_clavicle_ri", "j_counter", "j_dial_left", "j_dial_right",
    "j_elbow_le", "j_elbow_ri", "j_gun", "j_head", "j_helmet", "j_hip_le", "j_hip_ri",
    "j_knee_le", "j_knee_ri", "j_mainroot", "j_neck", "j_palm_ri", "j_shoulder_le", "j_shoulder_ri",
    "j_spine4", "j_spinelower", "j_spineupper", "j_wrist_le", "j_wrist_ri",
    "j_wristtwist_le", "j_wristtwist_ri",
};
const int zirconium::BONE_NAME_COUNT = sizeof(zirconium::BONE_NAMES) / sizeof(zirconium::BONE_NAMES[0]);

//  Indices into BONE_NAMES (alphabetical order):
//   0: j_ankle_le       1: j_ankle_ri      2: j_ball_le       3: j_ball_ri
//   4: j_barrel         5: j_clavicle_le   6: j_clavicle_ri   7: j_counter
//   8: j_dial_left      9: j_dial_right   10: j_elbow_le      11: j_elbow_ri
//  12: j_gun           13: j_head         14: j_helmet        15: j_hip_le
//  16: j_hip_ri        17: j_knee_le      18: j_knee_ri       19: j_mainroot
//  20: j_neck          21: j_palm_ri      22: j_shoulder_le   23: j_shoulder_ri
//  24: j_spine4        25: j_spinelower   26: j_spineupper    27: j_wrist_le
//  28: j_wrist_ri      29: j_wristtwist_le 30: j_wristtwist_ri

const int zirconium::BONE_CONNECTIONS[][2] = {
    // spine chain: root -> spinelower -> spineupper -> spine4 -> neck -> head
    {19, 25}, {25, 26}, {26, 24}, {24, 20}, {20, 13},
    // right arm: spine4 -> clavicle_ri -> shoulder_ri -> elbow_ri -> wrist_ri
    {24, 6},  {6, 23},  {23, 11}, {11, 28},
    // left arm: spine4 -> clavicle_le -> shoulder_le -> elbow_le -> wrist_le
    {24, 5},  {5, 22},  {22, 10}, {10, 27},
    // right leg: root -> hip_ri -> knee_ri -> ankle_ri -> ball_ri
    {19, 16}, {16, 18}, {18, 1},  {1, 3},
    // left leg: root -> hip_le -> knee_le -> ankle_le -> ball_le
    {19, 15}, {15, 17}, {17, 0},  {0, 2},
};
const int zirconium::BONE_CONNECTION_COUNT = sizeof(zirconium::BONE_CONNECTIONS) / sizeof(zirconium::BONE_CONNECTIONS[0]);

bool zirconium::AddressBook::resolve() {
    LOG_INFO("-- Address book init -- ");
    moduleBase = zirconium::memory_resolveModuleAddress(nullptr, 0);
    if (moduleBase == 0) {
        LOG_ERROR("Failed to get module base address.");
        return false;
    }
    LOG_INFO("Got module base address: 0x%p", (void*)moduleBase);

    uintptr_t playerBasePtr = moduleBase + OFF_PLAYER_BASE_PTR;
    if (playerBasePtr != 0) {
        playerBase = *reinterpret_cast<uintptr_t*>(playerBasePtr);
    }

    uintptr_t peBase = zirconium::memory_resolveModuleAddress(PE_MODULE_NAME, 0);

    // player
    playerHealth        = reinterpret_cast<unsigned int*>(peBase + OFF_PLAYER_HEALTH);
    playerHealthMax     = reinterpret_cast<unsigned int*>(peBase + OFF_PLAYER_HEALTH_MAX);
    playerPosX          = reinterpret_cast<float*>(OFF_PLAYER_POS_X);
    playerPosY          = reinterpret_cast<float*>(OFF_PLAYER_POS_Y);
    playerPosZ          = reinterpret_cast<float*>(OFF_PLAYER_POS_Z);
    playerGravity       = reinterpret_cast<unsigned int*>(playerBase + OFF_PB_GRAVITY);
    playerSpeed         = reinterpret_cast<unsigned int*>(playerBase + OFF_PB_SPEED);
    playerName          = reinterpret_cast<char*>(OFF_PLAYER_NAME);
    viewAngleX          = reinterpret_cast<float*>(OFF_PLAYER_VIEW_ANGLE_X);
    viewAngleY          = reinterpret_cast<float*>(OFF_PLAYER_VIEW_ANGLE_Y);

    // weapons/inventory
    primaryReserveAmmo  = reinterpret_cast<unsigned int*>(peBase + OFF_PRIMARY_RESERVE_AMMO);
    secondaryReserveAmmo = reinterpret_cast<unsigned int*>(OFF_SECONDARY_RESERVE_AMMO); // absolute
    grenades            = reinterpret_cast<unsigned int*>(peBase + OFF_GRENADES);
    claymores           = reinterpret_cast<unsigned int*>(peBase + OFF_CLAYMORES);
    monkeyBombs         = reinterpret_cast<unsigned int*>(peBase + OFF_MONKEY_BOMBS);
    money               = reinterpret_cast<unsigned int*>(peBase + OFF_MONEY);

    // game state
    zombieRound         = reinterpret_cast<unsigned int*>(peBase + OFF_ZOMBIE_ROUND);
    zombiesSpawned      = reinterpret_cast<unsigned int*>(peBase + OFF_ZOMBIES_SPAWNED);

    // entity list + view matrix
    zmEntityList        = reinterpret_cast<Zombie*>(ADDR_ZM_ENTITY_LIST_BASE);
    viewMatrix          = reinterpret_cast<float*>(ADDR_VIEW_MATRIX);

    // game functions
    Com_GetClientDObj         = reinterpret_cast<fn_Com_GetClientDObj>(ADDR_COM_GET_CLIENT_DOBJ);
    CG_DObjGetWorldBoneMatrix = reinterpret_cast<fn_CG_DObjGetWorldBoneMatrix>(ADDR_CG_DOBJ_GET_WORLD_BONE_MATRIX);
    DObjGetBoneIndex          = reinterpret_cast<fn_DObjGetBoneIndex>(ADDR_DOBJ_GET_BONE_INDEX);
    SL_FindString             = reinterpret_cast<fn_SL_FindString>(ADDR_SL_FIND_STRING);
    CG_GetEntity              = reinterpret_cast<fn_CG_GetEntity>(ADDR_CG_GET_ENTITY);

    LOG_INFO("Resolved game function pointers:");
    LOG_INFO("  Com_GetClientDObj         = 0x%p", (void*)Com_GetClientDObj);
    LOG_INFO("  CG_DObjGetWorldBoneMatrix = 0x%p", (void*)CG_DObjGetWorldBoneMatrix);
    LOG_INFO("  DObjGetBoneIndex          = 0x%p", (void*)DObjGetBoneIndex);
    LOG_INFO("  SL_FindString             = 0x%p", (void*)SL_FindString);
    LOG_INFO("  CG_GetEntity              = 0x%p", (void*)CG_GetEntity);

    LOG_INFO("-- Address book done -- ");

    resolved = true;
    return true;
}

bool zirconium::AddressBook::resolveBoneIDs() {
    if (!SL_FindString) {
        LOG_ERROR("SL_FindString is not resolved, cannot resolve bone IDs.");
        return false;
    }

    for (int i = 0; i < BONE_NAME_COUNT; i++) {
        boneStringIDs[i] = SL_FindString(BONE_NAMES[i]);
        if (boneStringIDs[i] == 0) {
            LOG_ERROR("SL_FindString failed for bone '%s' (idx %d)", BONE_NAMES[i], i);
        } else {
            LOG_DEBUG("(bone %u out of %u) Bone '%s' -> SL string ID %d",
                i, BONE_NAME_COUNT, BONE_NAMES[i], boneStringIDs[i]);
        }
    }
    LOG_INFO("Bones have been resolved");
    bonesResolved = true;
    return true;
}

void __stdcall zirconium::game_drawESP()
{
    if (!g_addrs.resolved) {
        LOG_ERROR("Tried to draw ESP but address book hasn't been initialized yet");
        return;
    }
    if (!g_addrs.zmEntityList) {
        LOG_ERROR("Tried to draw ESP zombie entity list is NULL");
        return;
    }
    if (!g_addrs.viewMatrix) {
        LOG_ERROR("Tried to draw ESP but player view matrix is NULL");
        return;
    }

    switch (g_overlayCtx.espRenderType)
    {
        case ESP_RENDER_TYPE_BOX:
            zirconium::game_drawBoxESP();
            break;
        case ESP_RENDER_TYPE_SKELETON:
            zirconium::game_drawSkeletonESP();
            break;
        case ESP_RENDER_TYPE_LINE:
            // todo
            break;
    }
}

void __stdcall zirconium::game_drawBoxESP()
{
    mat4f viewMatrix = mat4f::math_read(g_addrs.viewMatrix);

    float screenWidth = static_cast<float>(g_renderCtx.rect.right  - g_renderCtx.rect.left);
    float screenHeight = static_cast<float>(g_renderCtx.rect.bottom - g_renderCtx.rect.top);
    if (screenWidth <= 0.0f || screenHeight <= 0.0f) {
        return;
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (int i = 0; i < ZM_ENTITY_LIST_SZ; i++) {
        const Zombie& zm = g_addrs.zmEntityList[i];

        if (!zm.isAlive()) {
            continue;
        }

        vec3f zmPosition     = zm.position;
        vec3f zmHeadPosition = { zmPosition.x(), zmPosition.y(), zmPosition.z() + ZOMBIE_BOX_HEIGHT };

        vec2f screenFeet     = {0};
        vec2f screenHead     = {0};

        if (!math_worldToScreen(zmPosition, viewMatrix, screenWidth, screenHeight, screenFeet)) {
            continue;
        }
        if (!math_worldToScreen(zmHeadPosition, viewMatrix, screenWidth, screenHeight, screenHead)) {
            continue;
        }

        float boxHeight = screenFeet.y() - screenHead.y();
        if (boxHeight < 2.0f) {
            continue;
        }

        float boxWidth = boxHeight * 0.5f;

        float hpRatio = static_cast<float>(zm.currentHealth) / static_cast<float>(zm.startHealth);

        ImU32 boxColor = IM_COL32(
            static_cast<int>((1.0f - hpRatio) * 255),
            static_cast<int>(hpRatio * 255),
            0, 255
        );

        ImVec2 boxTopLeft     = ImVec2(screenHead.x() - boxWidth * 0.5f, screenHead.y());
        ImVec2 boxBottomRight = ImVec2(screenHead.x() - boxWidth * 0.5f + boxWidth,
                                    screenHead.y() + boxHeight);

        drawList->AddRect(
            boxTopLeft,
            boxBottomRight,
            boxColor, 0.0f, 0, 2.0f
        );

        static char healthBuffer[16];
        static char zmIdxBuffer[16];

        std::snprintf(healthBuffer, sizeof(healthBuffer), "Health: %u", zm.currentHealth);
        std::snprintf(zmIdxBuffer, sizeof(zmIdxBuffer), "Zombie: %u", i);

        boxTopLeft.y -= 10;
        drawList->AddText(boxTopLeft, boxColor, healthBuffer);
        boxTopLeft.y -= 10;
        drawList->AddText(boxTopLeft, boxColor, zmIdxBuffer);
    }
}

void __stdcall zirconium::game_drawSkeletonESP()
{
    if (!g_addrs.bonesResolved) {
        if (!g_addrs.resolveBoneIDs()) {
            LOG_ERROR("Failed to resolve bone IDs, falling back to box ESP");
            game_drawBoxESP();
            return;
        }
    }

    mat4f viewMatrix = mat4f::math_read(g_addrs.viewMatrix);

    float screenWidth  = static_cast<float>(g_renderCtx.rect.right  - g_renderCtx.rect.left);
    float screenHeight = static_cast<float>(g_renderCtx.rect.bottom - g_renderCtx.rect.top);
    if (screenWidth <= 0.0f || screenHeight <= 0.0f) {
        return;
    }

    ImDrawList* drawList = ImGui::GetBackgroundDrawList();

    for (int i = 0; i < ZM_ENTITY_LIST_SZ; i++) {
        const Zombie& zm = g_addrs.zmEntityList[i];
        if (!zm.isAlive()) {
            continue;
        }

        int entityIdx = i + g_addrs.zombieEntityOffset;

        // CG_GetEntity returns centity_t* — cpose_t is at offset 0, so same pointer
        void* entity = g_addrs.CG_GetEntity(0, entityIdx);
        if (!entity) {
            continue;
        }

        char* dobj = g_addrs.Com_GetClientDObj(entityIdx, 0);
        if (!dobj) {
            continue;
        }

        float hpRatio = static_cast<float>(zm.currentHealth) / static_cast<float>(zm.startHealth);
        ImU32 boneColor = IM_COL32(
            static_cast<int>((1.0f - hpRatio) * 255),
            static_cast<int>(hpRatio * 255),
            0, 255
        );

        // resolve world positions for each bone
        vec3f boneWorldPositions[MAX_BONES] = {};
        bool  boneValid[MAX_BONES] = {};

        for (int b = 0; b < BONE_NAME_COUNT; b++) {
            if (g_addrs.boneStringIDs[b] == 0) {
                continue;
            }

            unsigned char boneIdx = 0;
            int result = g_addrs.DObjGetBoneIndex(
                reinterpret_cast<int>(dobj),
                g_addrs.boneStringIDs[b],
                &boneIdx,
                -1
            );
            if (result == 0) {
                continue;
            }

            float rotMatrix[9] = {};
            float worldPos[3]  = {};
            g_addrs.CG_DObjGetWorldBoneMatrix(
                entity,
                dobj,
                static_cast<int>(boneIdx),
                rotMatrix,
                worldPos,
                false
            );

            boneWorldPositions[b] = { worldPos[0], worldPos[1], worldPos[2] };
            boneValid[b] = true;
        }

        // draw lines between connected bones
        for (int c = 0; c < BONE_CONNECTION_COUNT; c++) {
            int from = BONE_CONNECTIONS[c][0];
            int to   = BONE_CONNECTIONS[c][1];

            if (!boneValid[from] || !boneValid[to]) {
                continue;
            }

            vec2f screenFrom = {0};
            vec2f screenTo   = {0};

            if (!math_worldToScreen(boneWorldPositions[from], viewMatrix, screenWidth, screenHeight, screenFrom)) {
                continue;
            }
            if (!math_worldToScreen(boneWorldPositions[to], viewMatrix, screenWidth, screenHeight, screenTo)) {
                continue;
            }

            drawList->AddLine(
                ImVec2(screenFrom.x(), screenFrom.y()),
                ImVec2(screenTo.x(), screenTo.y()),
                boneColor, 2.0f
            );
        }

        // head circle (j_head = index 13)
        if (boneValid[13]) {
            vec2f screenHead = {0};
            if (math_worldToScreen(boneWorldPositions[13], viewMatrix, screenWidth, screenHeight, screenHead)) {
                drawList->AddCircle(ImVec2(screenHead.x(), screenHead.y()), 6.0f, boneColor, 12, 2.0f);
            }
        }

        // label at root bone (j_mainroot = index 19)
        if (boneValid[19]) {
            vec2f screenRoot = {0};
            if (math_worldToScreen(boneWorldPositions[19], viewMatrix, screenWidth, screenHeight, screenRoot)) {
                static char zmLabel[32];
                std::snprintf(zmLabel, sizeof(zmLabel), "Zombie %d [%u HP]", i, zm.currentHealth);
                drawList->AddText(ImVec2(screenRoot.x() - 30, screenRoot.y() + 5), boneColor, zmLabel);
            }
        }
    }
}