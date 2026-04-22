#pragma once
#include "math_utils.h"
#include "building.h"
// ============================================================
//  light_source.h  –  Swinging spotlight heads on buildings.
// ============================================================

struct SpotLight {
    float posX = 0.f, posY = 0.f, posZ = 0.f; // roof mount base
    float baseAngle = 0.f;                    // resting yaw angle (radians around Y)
    float swingAngle = 0.f;                   // current offset from baseAngle (oscillates ±SWING_MAX)
    float phase = 0.f;                        // phase for sin oscillation (per-light, starts offset)
    float headOffset = 1.5f;                  // horizontal offset from pivot to lamp head
    float mountHeight = 0.7f;                 // vertical offset from roof to gimbal pivot
    float targetX = 0.f, targetY = 0.f, targetZ = 0.f;
    float colorR, colorG, colorB;

    float yaw() const;
    float pivotY() const;

    // Derived: current world-space position of the light head.
    float headX() const;
    float headY() const;
    float headZ() const;

    // Direction the light is pointing (toward its road target)
    Vec3 direction() const;
};

struct SpotMeshes {
    Mesh sphere;   // emissive bulb
    Mesh ring;     // reused by street-light housing collar
    Mesh arm;      // shared box primitive
    Mesh base;     // shared cylinder primitive
    Mesh housing;  // spotlight can
};

void spotlights_init(SpotLight* lights, int count,
                     const Building* buildings, int nBuildings,
                     SpotMeshes& meshes);

void spotlight_update(SpotLight& light, float dt);

// Returns the current world-space position of the light (for Phong uniform)
Vec3 spotlight_world_pos(const SpotLight& light);

// Draw the spotlight gimbal + mount using the main lit shader.
void spotlight_draw_gimbal(const SpotLight& light,
                           const SpotMeshes& meshes,
                           GLuint shader);

// Draw the emissive sphere marker (uses the emissive shader)
void spotlight_draw_marker(const SpotLight& light,
                           const SpotMeshes& meshes,
                           GLuint emissiveShader);
