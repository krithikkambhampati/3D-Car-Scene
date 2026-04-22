#include "collision.h"
#include <algorithm>

bool circle_rect_overlap(float cx, float cz, float r,
                         const Rect2D& rect, float eps)
{
    // Clamp circle center to the rectangle, find closest point
    float nearX = std::max(rect.minX, std::min(cx, rect.maxX));
    float nearZ = std::max(rect.minZ, std::min(cz, rect.maxZ));
    float dx = cx - nearX, dz = cz - nearZ;
    float dist2 = dx*dx + dz*dz;
    float threshold = r + eps;
    return dist2 < threshold * threshold;
}

bool outside_arena(float cx, float cz, float r,
                   float arenaHW, float arenaHD, float eps)
{
    float margin = r + eps;
    return (cx < -arenaHW + margin) || (cx > arenaHW - margin) ||
           (cz < -arenaHD + margin) || (cz > arenaHD - margin);
}
