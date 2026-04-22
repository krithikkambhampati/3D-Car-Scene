#pragma once
#include <glad/glad.h>
#include "mesh.h"
#include "collision.h"
// ============================================================
//  building.h  –  Buildings with texture, fan, and spotlight.
// ============================================================

struct Building {
    // World position of the base center (bottom centre)
    float posX, posZ;
    float width, depth, height; // full dimensions

    // Colour tint applied when not using texture
    float colorR, colorG, colorB;

    GLuint texture = 0;  // 0 = use colour only
    float  shininess = 16.f;

    // Collision footprint in XZ (computed at init)
    Rect2D footprint;

    // Direction toward track center (normalised, XZ)
    float facingX, facingZ;

    // Visual style controls (used to diversify facades)
    int styleId = 0;
    float accentR = 0.9f, accentG = 0.9f, accentB = 0.9f;
};

// Meshes shared by all buildings (created once)
struct BuildingMeshes {
    Mesh slab;    // unit cube for body / floor slabs
};

void buildings_init(Building* buildings, int count,
                    BuildingMeshes& meshes,
                    GLuint brickTex, GLuint woodTex,
                    GLuint stoneTex, GLuint plasterTex);

void building_draw(const Building& b,
                   const BuildingMeshes& meshes,
                   GLuint shader);
