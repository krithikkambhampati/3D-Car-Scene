#pragma once
#include <glad/glad.h>
#include "mesh.h"
#include "building.h"
// ============================================================
//  fan.h  –  Spinning fans/windmills at the top of buildings.
//            Each fan is on the building face nearest the road
//            and rotates around the outward-normal of that face.
// ============================================================

struct Fan {
    float angle = 0.f;       // current rotor angle (radians)
    float spinSpeed = 0.f;   // rad/s (shared global, but stored per fan)
};

struct FanMeshes {
    Mesh tower;
    Mesh beam;
    Mesh hub;
    Mesh sail;
    Mesh cap;
};

// Initialise mesh geometry (once, shared by all windmills)
void fan_meshes_init(FanMeshes& fm);

// Advance windmill rotor angle
void fan_update(Fan& fan, float dt);

// Draw one windmill mounted on building b near the road-facing roof edge.
void fan_draw(const Fan& fan, const Building& b,
              const FanMeshes& fm, GLuint shader);

// Global fan speed adjustment
extern float g_fanSpeed; // rad/s
void fan_increase_speed();
void fan_decrease_speed();
