#pragma once
#include "math_utils.h"
// ============================================================
//  collision.h  –  Simple bounding-volume collision checks.
//
//  The car uses an oriented bounding box (OBB) footprint.
//  Buildings and walls use axis-aligned rectangles in XZ.
// ============================================================

// Axis-aligned rectangle in XZ plane (for static obstacles)
struct Rect2D {
    float minX, maxX, minZ, maxZ;
};

// Return true when a circle (cx,cz,r) overlaps the rectangle + epsilon margin.
bool circle_rect_overlap(float cx, float cz, float r,
                         const Rect2D& rect, float eps = 0.1f);

// Return true when an oriented car bounding box overlaps the rectangle.
// carX, carZ: car center position
// carHeading: car rotation angle in radians
// carHalfW, carHalfH: half-width and half-height of car bounding box
// rect: axis-aligned rectangle
// eps: epsilon margin for overlap detection
bool obb_rect_overlap(float carX, float carZ, float carHeading,
                      float carHalfW, float carHalfH,
                      const Rect2D& rect, float eps = 0.1f);

// Check if position (cx,cz) with radius r is outside the arena boundary.
bool outside_arena(float cx, float cz, float r,
                   float arenaHW, float arenaHD, float eps = 0.1f);

