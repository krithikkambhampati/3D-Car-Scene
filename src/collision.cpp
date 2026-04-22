#include "collision.h"
#include <algorithm>
#include <cmath>

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

// Separating Axis Theorem (SAT) for OBB vs AABB collision
bool obb_rect_overlap(float carX, float carZ, float carHeading,
                      float carHalfW, float carHalfH,
                      const Rect2D& rect, float eps)
{
    // OBB (oriented bounding box) axes
    float cosA = cosf(carHeading);
    float sinA = sinf(carHeading);
    
    // OBB half-extents
    float halfW = carHalfW + eps;
    float halfH = carHalfH + eps;
    
    // AABB half-extents
    float rectHalfW = (rect.maxX - rect.minX) * 0.5f + eps;
    float rectHalfH = (rect.maxZ - rect.minZ) * 0.5f + eps;
    float rectCx = (rect.minX + rect.maxX) * 0.5f;
    float rectCz = (rect.minZ + rect.maxZ) * 0.5f;
    
    // Vector from OBB center to AABB center
    float dx = rectCx - carX;
    float dz = rectCz - carZ;
    
    // Project onto OBB's local axes
    float dxLocal = dx * cosA + dz * sinA;
    float dzLocal = -dx * sinA + dz * cosA;
    
    // Test OBB's X axis
    if (fabsf(dxLocal) > halfW + rectHalfW * fabsf(cosA) + rectHalfH * fabsf(sinA)) {
        return false;
    }
    
    // Test OBB's Z axis
    if (fabsf(dzLocal) > halfH + rectHalfW * fabsf(sinA) + rectHalfH * fabsf(cosA)) {
        return false;
    }
    
    // Test AABB's X axis
    float px = carX + halfW * cosA;
    float pz = carZ + halfW * sinA;
    float qx = carX - halfH * sinA;
    float qz = carZ + halfH * cosA;
    if (dx > 0.0f) {
        if (rect.minX - eps > std::max({carX, px, qx})) return false;
    } else {
        if (rect.maxX + eps < std::min({carX, px, qx})) return false;
    }
    
    // Test AABB's Z axis
    if (dz > 0.0f) {
        if (rect.minZ - eps > std::max({carZ, pz, qz})) return false;
    } else {
        if (rect.maxZ + eps < std::min({carZ, pz, qz})) return false;
    }
    
    return true;
}

bool outside_arena(float cx, float cz, float r,
                   float arenaHW, float arenaHD, float eps)
{
    float margin = r + eps;
    return (cx < -arenaHW + margin) || (cx > arenaHW - margin) ||
           (cz < -arenaHD + margin) || (cz > arenaHD - margin);
}
