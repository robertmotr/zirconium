#pragma once

#define _USE_MATH_DEFINES
#include <cmath>
#include "t6zm.exe.h"

namespace zirconium {

    inline bool math_worldToScreen(
        const vec3_t& worldPos,
        const float viewProj[4][4],
        float screenWidth,
        float screenHeight,
        vec2_t& screenOut)
    {
        
        // transform to clip space
        float clip[4];
        for (int r = 0; r < 4; ++r) {
            clip[r] = viewProj[r][0] * worldPos.v[0]
                    + viewProj[r][1] * worldPos.v[1]
                    + viewProj[r][2] * worldPos.v[2]
                    + viewProj[r][3];
        }

        if (clip[3] <= 0.0f) return false;

        float invW = 1.0f / clip[3];
        float ndcX = clip[0] * invW;
        float ndcY = clip[1] * invW;

        screenOut.v[0] = (ndcX + 1.0f) * 0.5f * screenWidth;
        screenOut.v[1] = (1.0f - ndcY) * 0.5f * screenHeight;
        return true;
    }

    // X = forward, Y = left, Z = up
    inline vec2_t math_calcAngles(const vec3_t& from, const vec3_t& to)
    {
        float dx = to.v[0] - from.v[0];
        float dy = to.v[1] - from.v[1];
        float dz = to.v[2] - from.v[2];

        float horizDist = sqrtf(dx * dx + dy * dy);

        float pitch = -(atan2f(dz, horizDist) * (180.0f / M_PI));
        float yaw   =   atan2f(dy, dx)        * (180.0f / M_PI);

        vec2_t result;
        result.v[0] = pitch;
        result.v[1] = yaw;
        return result;
    }

    inline float math_wrapAngle(float angle)
    {
        while (angle >  180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
    }

} // namespace zirconium
