#include "pch.h"

#include <optional>

#include "zirconium_game.h"
#include "zirconium_globals.h"
#include "zirconium_hook.h"     // PE_MODULE_NAME
#include "zirconium_memory.h"
#include "t6zm.exe.h"

zirconium::AddressBook zirconium::g_addrBook{};

bool zirconium::AddressBook::resolveStatic() {
    LOG_INFO("=== Address book static resolve ===");

    // weapons/inventory
    primaryMainAmmo           = reinterpret_cast<unsigned int*>(ADDR_PRIMARY_MAIN_AMMO);
    primaryReserveAmmo        = reinterpret_cast<unsigned int*>(ADDR_PRIMARY_RESERVE_AMMO);
    secondaryMainAmmo         = reinterpret_cast<unsigned int*>(ADDR_SECONDARY_MAIN_AMMO);
    secondaryReserveAmmo      = reinterpret_cast<unsigned int*>(ADDR_SECONDARY_RESERVE_AMMO);
    grenades                  = reinterpret_cast<unsigned int*>(ADDR_GRENADES);
    claymores                 = reinterpret_cast<unsigned int*>(ADDR_CLAYMORES);
    monkeyBombs               = reinterpret_cast<unsigned int*>(ADDR_MONKEY_BOMBS);
    money                     = reinterpret_cast<unsigned int*>(ADDR_MONEY);

    // game state
    zombieRound               = reinterpret_cast<unsigned int*>(ADDR_ZOMBIE_ROUND);
    zombiesSpawned            = reinterpret_cast<unsigned int*>(ADDR_ZOMBIES_SPAWNED);

    // view matrix
    viewMatrix                = reinterpret_cast<float*>(ADDR_VIEW_MATRIX);

    // game functions
    Com_GetClientDObj         = reinterpret_cast<zirconium::Com_GetClientDObj>(ADDR_FN_COM_GET_CLIENT_DOBJ);
    CG_DObjGetWorldBoneMatrix = reinterpret_cast<zirconium::CG_DObjGetWorldBoneMatrix>(ADDR_FN_CG_DOBJ_GET_WORLD_BONE_MATRIX);
    DObjGetBoneIndex          = reinterpret_cast<zirconium::DObjGetBoneIndex>(ADDR_FN_DOBJ_GET_BONE_INDEX);
    SL_FindString             = reinterpret_cast<zirconium::SL_FindString>(ADDR_FN_SL_FIND_STRING);
    CG_GetEntity              = reinterpret_cast<zirconium::CG_GetEntity>(ADDR_FN_CG_GET_ENTITY);
    SL_ConvertToString        = reinterpret_cast<zirconium::SL_ConvertToString>(ADDR_FN_SL_CONVERT_TO_STRING);
    CG_GetEntityBoneInfo      = reinterpret_cast<zirconium::CG_GetEntityBoneInfo>(ADDR_FN_CG_GET_ENTITY_BONE_INFO);
    DObjGetBoneName           = reinterpret_cast<zirconium::DObjGetBoneName>(ADDR_FN_DOBJ_GET_BONE_NAME);

    // gentity player will always be gentity[0] in local games where the client is also the server
    gentityPlayer             = reinterpret_cast<gentity_t*>(ADDR_GENTITY_ARRAY);

    // dvars
    dvarGravity               = *reinterpret_cast<dvar_t**>(ADDR_DVAR_GRAVITY);
    dvarSpeed                 = *reinterpret_cast<dvar_t**>(ADDR_DVAR_SPEED);
    dvarJumpHeight            = *reinterpret_cast<dvar_t**>(ADDR_DVAR_JUMP_HEIGHT);

    resolved = true;
    LOG_INFO("=== Address book static resolve done ===");
    return true;
}

bool zirconium::AddressBook::refreshVolatile() {
    clientGame       = *reinterpret_cast<cg_t**>(ADDR_CLIENT_GAME_STRUCT);
    clientGameStatic = *reinterpret_cast<cgs_t**>(ADDR_CLIENT_GAME_STATIC_STRUCT);
    clientActive     = *reinterpret_cast<clientActive_t**>(ADDR_CLIENT_ACTIVE_STRUCT);
    centityPlayer    = CG_GetEntity ? CG_GetEntity(0, 0) : nullptr;

    return clientGame != nullptr && clientActive != nullptr;
}

int __cdecl zirconium::godModeHook(unsigned int /*a1*/) {
    return 0;
}

float* __cdecl zirconium::noRecoilHook(int /*a1*/, float* a2, float* a3, DWORD* /*a4*/, char /*a5*/, float* /*a6*/) {
    a3[0] = 0.f;
    a3[1] = 0.f;
    a3[2] = 0.f;
    return a2;
}

int __cdecl zirconium::noSpreadHook(int /*a1*/, int /*a2*/, float* x, float* y) {
    if (x) *x = 0.0f;
    if (y) *y = 0.0f;
    return 0;
}

namespace {

using zirconium::AIMBOT_BONE_OPTIONS;
using zirconium::BoneIndex;
using zirconium::BONE_LINKS;
using zirconium::BONE_LINK_COUNT;
using zirconium::BONE_HEAD_BASE;
using zirconium::BONE_HEAD_FOREHEAD;
using zirconium::BONE_PELVIS;
using zirconium::AimbotTargetMode;
using zirconium::AIMBOT_TARGET_MODE_NEAREST;
using zirconium::AIMBOT_TARGET_MODE_CROSSHAIR;
using zirconium::AIMBOT_TARGET_MODE_LOWEST_HP;
using zirconium::AIMBOT_TARGET_MODE_HIGHEST_HP;
using zirconium::MAX_CLIENT_ENTITIES;
using zirconium::MAX_REASONABLE_BONES;
using zirconium::ZM_ENTITY_FIRST_SLOT;

struct ScreenProjection {
    float viewMatrix[4][4];
    float width;
    float height;
};

struct ZombieTarget {
    uint32_t   entIdx;
    centity_t* centity;
    DObj*      dobj;
    int        numBones;
    bool       hasHealth;
    int        health;
    int        maxHealth;
    float      healthRatio;
    gentity_t* gentity;
};

struct AimContext {
    bool              valid;
    cg_t*             clientGame;
    clientActive_t*   clientActive;
    vec3_t            cameraOrigin;
    const vec3_t*     cameraAxes;       // forward, right, up
    ScreenProjection  screen;
    bool              lanMode;
    float             adsFraction;
};

struct AimCandidate {
    uint32_t entIdx;
    vec3_t   bonePos;
    vec2_t   boneScreen;
    bool     boneScreenValid;
    float    score;
};

struct ViewDelta {
    float pitch;
    float yaw;
};

// Reimplementation of CG_GetEntityBoneInfo. Returns 1 on success.
int __stdcall getBoneEntityInfo(const DObj      *dobj,
                                const centity_t *centity,
                                int              boneIndex,
                                vec3_t          *bonePos,
                                vec3_t          *boneAxis)
{
    if (boneIndex < 0) return 0;

    int boneOffset = 0;
    for (int modelIdx = 0; modelIdx < dobj->numModels; modelIdx++) {
        XModel *model = dobj->___u15.models[modelIdx];
        if (!model) continue;

        int numBones  = model->numBones;
        int localBone = boneIndex - boneOffset;

        if (localBone < numBones) {
            if (!zirconium::g_addrBook.CG_DObjGetWorldBoneMatrix(
                    &centity->pose,
                    const_cast<DObj*>(dobj),
                    boneIndex,
                    boneAxis,
                    bonePos,
                    false))
                return 0;
            return 1;
        }

        boneOffset += numBones;
        if (boneIndex < boneOffset) return 0;
    }
    return 0;
}

ScreenProjection makeScreenProjection(const float* rawViewMatrix, const RECT& rect) {
    ScreenProjection s{};
    std::memcpy(s.viewMatrix, rawViewMatrix, sizeof(s.viewMatrix));
    s.width  = static_cast<float>(rect.right  - rect.left);
    s.height = static_cast<float>(rect.bottom - rect.top);
    return s;
}

bool projectToScreen(const vec3_t& world, const ScreenProjection& s, vec2_t& out) {
    return zirconium::math_worldToScreen(world, s.viewMatrix, s.width, s.height, out);
}

std::optional<ZombieTarget> tryResolveZombie(uint32_t entIdx, bool lanMode) {
    centity_t* centity = zirconium::g_addrBook.CG_GetEntity(0, entIdx);
    if (!centity) return std::nullopt;

    unsigned __int8 eType = centity->pose.eType;
    if (eType != ET_ACTOR) return std::nullopt;
    if (static_cast<unsigned int>(centity->nextState.lerp.eFlags) >= zirconium::EF_DEAD_THRESHOLD) return std::nullopt;

    ZombieTarget t{};
    t.entIdx      = entIdx;
    t.centity     = centity;
    t.gentity     = nullptr;
    t.hasHealth   = false;
    t.healthRatio = 1.0f;

    if (lanMode) {
        gentity_t* gentity = reinterpret_cast<gentity_t*>(ADDR_GENTITY_ARRAY + sizeof(gentity_t) * entIdx);
        if (!gentity || gentity->health <= 0 || gentity->maxHealth <= 0) return std::nullopt;

        t.gentity     = gentity;
        t.health      = gentity->health;
        t.maxHealth   = gentity->maxHealth;
        t.healthRatio = static_cast<float>(gentity->health) / static_cast<float>(gentity->maxHealth);
        if (t.healthRatio > 1.0f) t.healthRatio = 1.0f;
        t.hasHealth   = true;
    }

    DObj* dobj = zirconium::g_addrBook.Com_GetClientDObj(entIdx, 0);
    if (!dobj) return std::nullopt;

    // DObj can be mid-rebuild during third-person toggle or pause and return
    // a garbage numBones value. Reject obviously-corrupt counts.
    int numBones = dobj->numBones;
    if (numBones <= 0 || numBones > MAX_REASONABLE_BONES) return std::nullopt;

    t.dobj     = dobj;
    t.numBones = numBones;
    return t;
}

ImU32 zombieColourForHealth(const ZombieTarget& t) {
    if (!t.hasHealth) return IM_COL32(0, 255, 0, 255);
    int red   = 255 - static_cast<int>(t.healthRatio * 255);
    int green = static_cast<int>(t.healthRatio * 255);
    return IM_COL32(red, green, 0, 255);
}

void drawSkeleton(const ZombieTarget& t, const ScreenProjection& screen,
                  ImDrawList* draw, ImU32 colour)
{
    for (int i = 0; i < BONE_LINK_COUNT; i++) {
        int boneFromIdx = static_cast<int>(BONE_LINKS[i].from);
        int boneToIdx   = static_cast<int>(BONE_LINKS[i].to);
        if (boneFromIdx >= t.numBones || boneToIdx >= t.numBones) continue;

        vec3_t boneFromWorld = {}, boneToWorld = {};
        vec3_t axisFrom[3]   = {}, axisTo[3]   = {};

        if (!getBoneEntityInfo(t.dobj, t.centity, boneFromIdx, &boneFromWorld, axisFrom)) continue;
        if (!getBoneEntityInfo(t.dobj, t.centity, boneToIdx,   &boneToWorld,   axisTo))   continue;

        vec2_t boneFromScreen = {}, boneToScreen = {};
        if (!projectToScreen(boneFromWorld, screen, boneFromScreen)) continue;
        if (!projectToScreen(boneToWorld,   screen, boneToScreen))   continue;

        draw->AddLine(
            ImVec2(boneFromScreen.v[0], boneFromScreen.v[1]),
            ImVec2(boneToScreen.v[0],   boneToScreen.v[1]),
            colour, 1.5f);
    }
}

void drawHeadCircle(const ZombieTarget& t, const ScreenProjection& screen,
                    ImDrawList* draw, ImU32 colour)
{
    if (BONE_HEAD_BASE >= t.numBones || BONE_HEAD_FOREHEAD >= t.numBones) return;

    vec3_t baseWorld = {}, foreheadWorld = {};
    vec3_t baseAxis[3] = {}, foreheadAxis[3] = {};

    if (!getBoneEntityInfo(t.dobj, t.centity, BONE_HEAD_BASE,     &baseWorld,     baseAxis))     return;
    if (!getBoneEntityInfo(t.dobj, t.centity, BONE_HEAD_FOREHEAD, &foreheadWorld, foreheadAxis)) return;

    vec2_t baseScreen = {}, foreheadScreen = {};
    if (!projectToScreen(baseWorld,     screen, baseScreen))     return;
    if (!projectToScreen(foreheadWorld, screen, foreheadScreen)) return;

    float cx = (baseScreen.v[0] + foreheadScreen.v[0]) * 0.5f;
    float cy = (baseScreen.v[1] + foreheadScreen.v[1]) * 0.5f;
    float dx = foreheadScreen.v[0] - baseScreen.v[0];
    float dy = foreheadScreen.v[1] - baseScreen.v[1];
    float radius = sqrtf(dx * dx + dy * dy) * 0.5f;

    draw->AddCircle(ImVec2(cx, cy), radius, colour, 0, 1.5f);
}

struct ScreenAABB {
    float rightEdge;
    float topEdge;
    float bottomEdge;
    float centerX;
    int   visibleCorners;
};

bool projectAABB(const vec3_t& mins, const vec3_t& maxs,
                 const ScreenProjection& screen, ScreenAABB& out)
{
    out.rightEdge      = -FLT_MAX;
    out.topEdge        =  FLT_MAX;
    out.bottomEdge     = -FLT_MAX;
    out.centerX        =  0.0f;
    out.visibleCorners =  0;

    for (int corner = 0; corner < 8; corner++) {
        vec3_t worldCorner;
        worldCorner.v[0] = (corner & 1) ? maxs.v[0] : mins.v[0];
        worldCorner.v[1] = (corner & 2) ? maxs.v[1] : mins.v[1];
        worldCorner.v[2] = (corner & 4) ? maxs.v[2] : mins.v[2];

        vec2_t screenCorner = {};
        if (!projectToScreen(worldCorner, screen, screenCorner)) continue;

        if (screenCorner.v[0] > out.rightEdge)  out.rightEdge  = screenCorner.v[0];
        if (screenCorner.v[1] < out.topEdge)    out.topEdge    = screenCorner.v[1];
        if (screenCorner.v[1] > out.bottomEdge) out.bottomEdge = screenCorner.v[1];
        out.centerX += screenCorner.v[0];
        out.visibleCorners++;
    }
    if (out.visibleCorners == 0) return false;
    out.centerX /= out.visibleCorners;
    return true;
}

void drawZombieLabelAndHealthBar(const ZombieTarget& t, const ScreenProjection& screen,
                                 ImDrawList* draw)
{
    ScreenAABB aabb;
    if (!projectAABB(t.centity->pose.absmin, t.centity->pose.absmax, screen, aabb)) return;

    char labelText[16];
    std::snprintf(labelText, sizeof(labelText), "Zombie %u", t.entIdx - ZM_ENTITY_FIRST_SLOT);
    ImVec2 labelSize = ImGui::CalcTextSize(labelText);
    ImVec2 labelPos(aabb.centerX - labelSize.x * 0.5f, aabb.topEdge - labelSize.y - 4.0f);

    draw->AddRectFilled(
        ImVec2(labelPos.x - 2, labelPos.y - 1),
        ImVec2(labelPos.x + labelSize.x + 2, labelPos.y + labelSize.y + 1),
        IM_COL32(0, 0, 0, 160));
    draw->AddText(labelPos, IM_COL32(255, 255, 255, 255), labelText);

    if (!t.hasHealth) return;

    constexpr float barWidth   = 4.0f;
    constexpr float barPadding = 6.0f;

    float barLeft   = aabb.rightEdge + barPadding;
    float barTop    = aabb.topEdge;
    float barBottom = aabb.bottomEdge;
    float barHeight = barBottom - barTop;

    draw->AddRectFilled(
        ImVec2(barLeft - 1, barTop - 1),
        ImVec2(barLeft + barWidth + 1, barBottom + 1),
        IM_COL32(0, 0, 0, 180));
    draw->AddRectFilled(
        ImVec2(barLeft, barTop),
        ImVec2(barLeft + barWidth, barBottom),
        IM_COL32(200, 0, 0, 255));

    float filledHeight = barHeight * t.healthRatio;
    draw->AddRectFilled(
        ImVec2(barLeft, barBottom - filledHeight),
        ImVec2(barLeft + barWidth, barBottom),
        IM_COL32(0, 200, 0, 255));

    char healthText[16];
    std::snprintf(healthText, sizeof(healthText), "%.0f%%", t.healthRatio * 100.0f);
    ImVec2 healthSize = ImGui::CalcTextSize(healthText);
    ImVec2 healthPos(barLeft + barWidth + 4.0f, barTop + barHeight * 0.5f - healthSize.y * 0.5f);

    draw->AddRectFilled(
        ImVec2(healthPos.x - 2, healthPos.y - 1),
        ImVec2(healthPos.x + healthSize.x + 2, healthPos.y + healthSize.y + 1),
        IM_COL32(0, 0, 0, 160));
    draw->AddText(healthPos, IM_COL32(255, 255, 255, 255), healthText);
}

void drawSnapLine(const ZombieTarget& t, const ScreenProjection& screen,
                  const vec3_t& playerOrigin, ImDrawList* draw, ImU32 colour)
{
    if (BONE_PELVIS >= t.numBones) return;

    vec3_t pelvisWorld = {};
    vec3_t pelvisAxis[3] = {};
    if (!getBoneEntityInfo(t.dobj, t.centity, BONE_PELVIS, &pelvisWorld, pelvisAxis)) return;

    vec2_t pelvisScreen = {};
    if (!projectToScreen(pelvisWorld, screen, pelvisScreen)) return;

    ImVec2 screenBottom(screen.width * 0.5f, screen.height);
    ImVec2 zombiePoint(pelvisScreen.v[0], pelvisScreen.v[1]);
    draw->AddLine(screenBottom, zombiePoint, colour, 1.0f);

    float dx = pelvisWorld.v[0] - playerOrigin.v[0];
    float dy = pelvisWorld.v[1] - playerOrigin.v[1];
    float dz = pelvisWorld.v[2] - playerOrigin.v[2];
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    char distBuf[16];
    std::snprintf(distBuf, sizeof(distBuf), "%.0f", dist);
    ImVec2 distSize = ImGui::CalcTextSize(distBuf);

    ImVec2 midpoint((screenBottom.x + zombiePoint.x) * 0.5f,
                    (screenBottom.y + zombiePoint.y) * 0.5f);

    draw->AddRectFilled(
        ImVec2(midpoint.x - distSize.x * 0.5f - 2, midpoint.y - 1),
        ImVec2(midpoint.x + distSize.x * 0.5f + 2, midpoint.y + distSize.y + 1),
        IM_COL32(0, 0, 0, 160));
    draw->AddText(
        ImVec2(midpoint.x - distSize.x * 0.5f, midpoint.y),
        IM_COL32(255, 255, 255, 255), distBuf);
}

// =================================================================
// Debug overlays (Track 4)
// =================================================================

constexpr ImU32 DEBUG_TEXT_COL = IM_COL32( 50, 230, 230, 255);
constexpr ImU32 DEBUG_BG_COL   = IM_COL32(  0,   0,   0, 180);
constexpr ImU32 DEBUG_ACCENT   = IM_COL32( 50, 230, 230, 200);

void drawDebugTextBlock(ImDrawList* draw, float anchorX, float anchorY,
                        const char* const* lines, int lineCount)
{
    float maxW = 0.0f;
    float lineH = ImGui::GetTextLineHeight();
    for (int i = 0; i < lineCount; i++) {
        ImVec2 sz = ImGui::CalcTextSize(lines[i]);
        if (sz.x > maxW) maxW = sz.x;
    }
    float totalH = lineH * lineCount + 2.0f;

    draw->AddRectFilled(
        ImVec2(anchorX - 2.0f,        anchorY - 1.0f),
        ImVec2(anchorX + maxW + 2.0f, anchorY + totalH),
        DEBUG_BG_COL);

    for (int i = 0; i < lineCount; i++) {
        draw->AddText(ImVec2(anchorX, anchorY + i * lineH),
                      DEBUG_TEXT_COL, lines[i]);
    }
}

void drawEspDebugBlock(const ZombieTarget& t, const ScreenProjection& screen,
                       const vec3_t& playerOrigin, ImDrawList* draw)
{
    ScreenAABB aabb;
    if (!projectAABB(t.centity->pose.absmin, t.centity->pose.absmax, screen, aabb)) return;

    float dx = t.centity->pose.absmax.v[0] - playerOrigin.v[0];
    float dy = t.centity->pose.absmax.v[1] - playerOrigin.v[1];
    float dz = t.centity->pose.absmax.v[2] - playerOrigin.v[2];
    float dist = sqrtf(dx * dx + dy * dy + dz * dz);

    char lines[40][96];
    int  n = 0;
    std::snprintf(lines[n++], sizeof(lines[0]), "entIdx    : %u (0x%X)", t.entIdx, t.entIdx);
    std::snprintf(lines[n++], sizeof(lines[0]), "centity   : %p",        (void*)t.centity);
    std::snprintf(lines[n++], sizeof(lines[0]), "eType     : %u",        (unsigned)t.centity->pose.eType);
    std::snprintf(lines[n++], sizeof(lines[0]), "eTypeU    : %u",        (unsigned)t.centity->pose.eTypeUnion);
    std::snprintf(lines[n++], sizeof(lines[0]), "isRagdoll : %u",        (unsigned)t.centity->pose.isRagdoll);
    std::snprintf(lines[n++], sizeof(lines[0]), "ragdollH  : %d",        t.centity->pose.ragdollHandle);
    std::snprintf(lines[n++], sizeof(lines[0]), "absmin    : (%.1f %.1f %.1f)",
                  t.centity->pose.absmin.v[0], t.centity->pose.absmin.v[1], t.centity->pose.absmin.v[2]);
    std::snprintf(lines[n++], sizeof(lines[0]), "absmax    : (%.1f %.1f %.1f)",
                  t.centity->pose.absmax.v[0], t.centity->pose.absmax.v[1], t.centity->pose.absmax.v[2]);
    std::snprintf(lines[n++], sizeof(lines[0]), "dist      : %.1f",      dist);

    std::snprintf(lines[n++], sizeof(lines[0]), "-- DObj --");
    std::snprintf(lines[n++], sizeof(lines[0]), "dobj      : %p",        (void*)t.dobj);
    std::snprintf(lines[n++], sizeof(lines[0]), "entnum    : %u",        (unsigned)t.dobj->entnum);
    std::snprintf(lines[n++], sizeof(lines[0]), "numBones  : %u",        (unsigned)t.dobj->numBones);
    std::snprintf(lines[n++], sizeof(lines[0]), "numModels : %u",        (unsigned)t.dobj->numModels);
    std::snprintf(lines[n++], sizeof(lines[0]), "flags     : 0x%02X",    (unsigned)t.dobj->flags);
    std::snprintf(lines[n++], sizeof(lines[0]), "locked    : %d",        t.dobj->locked);
    std::snprintf(lines[n++], sizeof(lines[0]), "radius    : %.1f",      t.dobj->radius);
    std::snprintf(lines[n++], sizeof(lines[0]), "skel ts   : %d",        t.dobj->skel.timeStamp);
    std::snprintf(lines[n++], sizeof(lines[0]), "animTree  : %p",        (void*)t.dobj->___u0.tree);

    std::snprintf(lines[n++], sizeof(lines[0]), "-- nextState --");
    std::snprintf(lines[n++], sizeof(lines[0]), "ns.eType   : %d",       (int)t.centity->nextState.eType);
    std::snprintf(lines[n++], sizeof(lines[0]), "ns.eFlags  : 0x%08X",   (unsigned)t.centity->nextState.lerp.eFlags);
    std::snprintf(lines[n++], sizeof(lines[0]), "ns.eFlags2 : 0x%08X",   (unsigned)t.centity->nextState.lerp.eFlags2);
    std::snprintf(lines[n++], sizeof(lines[0]), "ps.eFlags  : 0x%08X",   (unsigned)t.centity->prevState.eFlags);
    std::snprintf(lines[n++], sizeof(lines[0]), "animState  : %d / %u",  (int)t.centity->nextState.un2.animState.state,
                                                                          (unsigned)t.centity->nextState.un2.animState.subState);
    std::snprintf(lines[n++], sizeof(lines[0]), "actor team : %d",       t.centity->nextState.lerp.u.actor.team);
    std::snprintf(lines[n++], sizeof(lines[0]), "actor enmy : %d",       t.centity->nextState.lerp.u.actor.enemy);
    std::snprintf(lines[n++], sizeof(lines[0]), "actor aiT  : %u",       (unsigned)t.centity->nextState.lerp.u.actor.aiType);
    std::snprintf(lines[n++], sizeof(lines[0]), "c.tree     : %p",       (void*)t.centity->tree);
    std::snprintf(lines[n++], sizeof(lines[0]), "flagsBits  : 0x%08X",   (unsigned)t.centity->___u38.packed_bits[0]);

    if (t.hasHealth && t.gentity) {
        std::snprintf(lines[n++], sizeof(lines[0]), "-- LAN --");
        std::snprintf(lines[n++], sizeof(lines[0]), "gentity   : %p",      (void*)t.gentity);
        std::snprintf(lines[n++], sizeof(lines[0]), "hp        : %d / %d", t.health, t.maxHealth);
        std::snprintf(lines[n++], sizeof(lines[0]), "sentient  : %p",      (void*)t.gentity->sentient);
    }

    const char* lineptrs[40];
    for (int i = 0; i < n; i++) lineptrs[i] = lines[i];

    float anchorX = aabb.rightEdge + 60.0f;
    float anchorY = aabb.topEdge;
    drawDebugTextBlock(draw, anchorX, anchorY, lineptrs, n);
}

void drawAimbotDebugBlock(const AimCandidate& best, BoneIndex bone,
                          AimbotTargetMode mode, const AimContext& aim,
                          const ViewDelta& delta, ImDrawList* draw)
{
    if (best.boneScreenValid) {
        draw->AddCircle(ImVec2(best.boneScreen.v[0], best.boneScreen.v[1]),
                        9.0f, DEBUG_ACCENT, 0, 2.0f);
        draw->AddLine(ImVec2(aim.screen.width * 0.5f, aim.screen.height * 0.5f),
                      ImVec2(best.boneScreen.v[0], best.boneScreen.v[1]),
                      DEBUG_ACCENT, 1.0f);
    }

    char lines[10][96];
    int n = 0;
    std::snprintf(lines[n++], sizeof(lines[0]), "entIdx   : %u",         best.entIdx);
    std::snprintf(lines[n++], sizeof(lines[0]), "score    : %.3f",       best.score);
    std::snprintf(lines[n++], sizeof(lines[0]), "bone     : %d (%s)",    (int)bone,
                  AIMBOT_BONE_OPTIONS[zirconium::g_overlayCtx.aimbotTargetBone].name);
    std::snprintf(lines[n++], sizeof(lines[0]), "mode     : %d",         (int)mode);
    std::snprintf(lines[n++], sizeof(lines[0]), "world    : (%.1f %.1f %.1f)",
                  best.bonePos.v[0], best.bonePos.v[1], best.bonePos.v[2]);
    if (best.boneScreenValid) {
        std::snprintf(lines[n++], sizeof(lines[0]), "screen   : (%.0f, %.0f)",
                      best.boneScreen.v[0], best.boneScreen.v[1]);
    } else {
        std::snprintf(lines[n++], sizeof(lines[0]), "screen   : offscreen");
    }
    std::snprintf(lines[n++], sizeof(lines[0]), "dPitch   : %+.3f",      delta.pitch);
    std::snprintf(lines[n++], sizeof(lines[0]), "dYaw     : %+.3f",      delta.yaw);
    std::snprintf(lines[n++], sizeof(lines[0]), "adsFrac  : %.3f",       aim.adsFraction);

    const char* lineptrs[10];
    for (int i = 0; i < n; i++) lineptrs[i] = lines[i];

    float anchorX = best.boneScreenValid ? best.boneScreen.v[0] + 14.0f : 12.0f;
    float anchorY = best.boneScreenValid ? best.boneScreen.v[1] + 14.0f : 12.0f;
    drawDebugTextBlock(draw, anchorX, anchorY, lineptrs, n);
}

// =================================================================
// Aimbot helpers
// =================================================================

AimContext tryLoadAimContext() {
    AimContext a{};
    a.valid = false;

    a.clientActive = zirconium::g_addrBook.clientActive;
    a.clientGame   = zirconium::g_addrBook.clientGame;
    if (!a.clientActive || !a.clientGame)   return a;
    if (!zirconium::g_addrBook.viewMatrix)  return a;

    a.adsFraction = a.clientGame->predictedPlayerState.fWeaponPosFrac;
    if (!(a.adsFraction > 0.0f)) return a;

    a.cameraOrigin = a.clientGame->refdef.vieworg;
    a.cameraAxes   = a.clientGame->refdef.viewaxis;
    a.screen       = makeScreenProjection(zirconium::g_addrBook.viewMatrix, zirconium::g_renderCtx.rect);
    a.lanMode      = zirconium::g_overlayCtx.isLanGame;
    a.valid        = true;
    return a;
}

float scoreCandidate(const ZombieTarget& t, const vec3_t& bonePos,
                     AimbotTargetMode mode, const AimContext& aim,
                     vec2_t& boneScreenOut, bool& boneScreenValidOut)
{
    boneScreenValidOut = projectToScreen(bonePos, aim.screen, boneScreenOut);

    switch (mode) {
        case AIMBOT_TARGET_MODE_NEAREST: {
            float dx = bonePos.v[0] - aim.cameraOrigin.v[0];
            float dy = bonePos.v[1] - aim.cameraOrigin.v[1];
            float dz = bonePos.v[2] - aim.cameraOrigin.v[2];
            return dx * dx + dy * dy + dz * dz;
        }
        case AIMBOT_TARGET_MODE_CROSSHAIR: {
            if (!boneScreenValidOut) return FLT_MAX;
            float dx = boneScreenOut.v[0] - aim.screen.width  * 0.5f;
            float dy = boneScreenOut.v[1] - aim.screen.height * 0.5f;
            return dx * dx + dy * dy;
        }
        case AIMBOT_TARGET_MODE_LOWEST_HP:
        case AIMBOT_TARGET_MODE_HIGHEST_HP: {
            if (!aim.lanMode || !t.gentity) return FLT_MAX;
            float hp = static_cast<float>(t.gentity->health);
            return (mode == AIMBOT_TARGET_MODE_LOWEST_HP) ? hp : -hp;
        }
        default: {
            float dx = bonePos.v[0] - aim.cameraOrigin.v[0];
            float dy = bonePos.v[1] - aim.cameraOrigin.v[1];
            float dz = bonePos.v[2] - aim.cameraOrigin.v[2];
            return dx * dx + dy * dy + dz * dz;
        }
    }
}

std::optional<AimCandidate> findBestTarget(const AimContext& aim,
                                           BoneIndex targetBone,
                                           AimbotTargetMode mode)
{
    AimCandidate best{};
    best.score = FLT_MAX;
    bool found = false;

    for (uint32_t entIdx = 1; entIdx < MAX_CLIENT_ENTITIES; entIdx++) {
        auto target = tryResolveZombie(entIdx, aim.lanMode);
        if (!target) continue;
        if (static_cast<int>(targetBone) >= target->numBones) continue;

        vec3_t bonePos = {};
        vec3_t boneAxis[3] = {};
        if (!getBoneEntityInfo(target->dobj, target->centity,
                               static_cast<int>(targetBone),
                               &bonePos, boneAxis)) continue;

        vec2_t boneScreen   = {};
        bool   boneScreenOk = false;
        float  score = scoreCandidate(*target, bonePos, mode, aim,
                                      boneScreen, boneScreenOk);

        if (score < best.score) {
            best.score           = score;
            best.entIdx          = entIdx;
            best.bonePos         = bonePos;
            best.boneScreen      = boneScreen;
            best.boneScreenValid = boneScreenOk;
            found = true;
        }
    }

    if (!found) return std::nullopt;
    return best;
}

// Approach history 2-7 (tried and discarded before settling on Approach 8):
//   2: predictedPlayerState.delta_angles       -> desync/flicker (prediction fight)
//   3: cl->viewangles absolute                 -> smooth but completely wrong direction
//   4: cl->viewangles + delta_angles hybrid    -> worst of both worlds
//   5: cl->viewangles absolute, yaw -90        -> close, ~1m to the right
//   6: cl->viewangles absolute, yaw +90        -> 180 flip
//   7: cl->viewangles delta-based              -> 90 degrees off
// All failed because math_calcAngles produces angles in world-space but
// cl->viewangles uses an unknown convention. Absolute and delta writes both
// had consistent angular offsets that varied with approach.
//
// Approach 8 (current): project the target direction onto camera-local axes
// (forward/right/up from refdef.viewaxis). Dot products give exact pitch/yaw
// delta regardless of the viewangles coordinate convention.
ViewDelta computeViewAngleDelta(const vec3_t& targetPos,
                                const vec3_t& cameraOrigin,
                                const vec3_t* cameraAxes)
{
    ViewDelta d{0.0f, 0.0f};

    float dirX = targetPos.v[0] - cameraOrigin.v[0];
    float dirY = targetPos.v[1] - cameraOrigin.v[1];
    float dirZ = targetPos.v[2] - cameraOrigin.v[2];

    float length = sqrtf(dirX * dirX + dirY * dirY + dirZ * dirZ);
    if (length < 0.001f) return d;

    float inv = 1.0f / length;
    dirX *= inv; dirY *= inv; dirZ *= inv;

    const vec3_t& forward = cameraAxes[0];
    const vec3_t& right   = cameraAxes[1];
    const vec3_t& up      = cameraAxes[2];

    float forwardComponent = dirX * forward.v[0] + dirY * forward.v[1] + dirZ * forward.v[2];
    float rightComponent   = dirX * right.v[0]   + dirY * right.v[1]   + dirZ * right.v[2];
    float upComponent      = dirX * up.v[0]      + dirY * up.v[1]      + dirZ * up.v[2];

    d.yaw   = atan2f(rightComponent, forwardComponent) * (180.0f / static_cast<float>(M_PI));
    d.pitch = atan2f(-upComponent,   forwardComponent) * (180.0f / static_cast<float>(M_PI));
    return d;
}

} // anonymous namespace


void __stdcall zirconium::game_drawESP()
{
    if (!g_addrBook.resolved)   { LOG_ERROR("ESP: address book not resolved"); return; }
    if (!g_addrBook.viewMatrix) { LOG_ERROR("ESP: view matrix null");          return; }
    if (!g_addrBook.clientGame) return;

    const ScreenProjection screen = makeScreenProjection(g_addrBook.viewMatrix, g_renderCtx.rect);
    ImDrawList* draw              = ImGui::GetBackgroundDrawList();
    const vec3_t& playerOrigin    = g_addrBook.clientGame->predictedPlayerState.origin;
    const bool lanMode            = g_overlayCtx.isLanGame;

    for (uint32_t entIdx = 1; entIdx < MAX_CLIENT_ENTITIES; entIdx++) {
        auto target = tryResolveZombie(entIdx, lanMode);
        if (!target) continue;

        const ImU32 colour = zombieColourForHealth(*target);

        drawSkeleton  (*target, screen, draw, colour);
        drawHeadCircle(*target, screen, draw, colour);

        if (g_overlayCtx.espRenderZombieInfo)
            drawZombieLabelAndHealthBar(*target, screen, draw);

        if (g_overlayCtx.espRenderSnapLines)
            drawSnapLine(*target, screen, playerOrigin, draw, colour);

        if (g_overlayCtx.visualDebug)
            drawEspDebugBlock(*target, screen, playerOrigin, draw);
    }
}

void __stdcall zirconium::game_aimbot()
{
    if (!g_addrBook.resolved) return;

    const AimContext aim = tryLoadAimContext();
    if (!aim.valid) return;

    const BoneIndex        targetBone = AIMBOT_BONE_OPTIONS[g_overlayCtx.aimbotTargetBone].index;
    const AimbotTargetMode mode       = static_cast<AimbotTargetMode>(g_overlayCtx.aimbotTargetMode);

    auto best = findBestTarget(aim, targetBone, mode);
    if (!best) return;

    const ViewDelta delta = computeViewAngleDelta(best->bonePos, aim.cameraOrigin, aim.cameraAxes);
    aim.clientActive->viewangles.v[0] += delta.pitch;
    aim.clientActive->viewangles.v[1] += delta.yaw;

    if (g_overlayCtx.visualDebug)
        drawAimbotDebugBlock(*best, targetBone, mode, aim, delta, ImGui::GetBackgroundDrawList());
}
