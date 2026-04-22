#pragma once
#include "math_utils.h"
// ============================================================
//  collision.h  –  Simple bounding-volume collision checks.
//
//  The car uses a horizontal bounding circle (footprint).
//  Buildings and walls use axis-aligned rectangles in XZ.
// ============================================================

// Axis-aligned rectangle in XZ plane (for static obstacles)
struct Rect2D {
    float minX, maxX, minZ, maxZ;
};

// Return true when a circle (cx,cz,r) overlaps the rectangle + epsilon margin.
bool circle_rect_overlap(float cx, float cz, float r,
                         const Rect2D& rect, float eps = 0.1f);

// Check if position (cx,cz) with radius r is outside the arena boundary.
bool outside_arena(float cx, float cz, float r,
                   float arenaHW, float arenaHD, float eps = 0.1f);
