#pragma once
#include <glad/glad.h>
#include "math_utils.h"
#include "mesh.h"
#include "collision.h"
#include "constants.h"
// ============================================================
//  car.h  –  Car state, geometry, and update logic.
// ============================================================

struct Car {
    // World-space position (car is on the XZ ground plane, y=0)
    float posX = 0.f, posZ = -(TRACK_B - 3.5f);  // start on track (inner part), facing +X direction
    float heading = 0.f;                 // angle in radians; 0 = facing +X
    float speed   = 0.f;                 // current speed (units/s); can be negative
    bool  stopped = false;               // locked after collision
    bool  headlightsOn = true;           // toggled with keyboard

    // Meshes (uploaded once at init)
    Mesh bodyMesh;
    Mesh cabinMesh;
    Mesh wheelMesh;
    Mesh headlightMesh;
    Mesh headlightBeamMesh;
    GLuint bodyTex = 0;
    GLuint metalTex = 0;
    GLuint rubberTex = 0;
};

// Initialise meshes (call after OpenGL context is ready).
void car_init(Car& car);

// Advance position along heading by speed*dt. Clamps speed to [MIN, MAX].
void car_update(Car& car, float dt);

// Draw all car parts using the currently-bound shader.
// Requires: uModel, uColor, uShininess, uUseTexture uniforms in shader.
void car_draw(const Car& car, GLuint shader);

// Change speed by delta (clamped to [CAR_MIN_SPEED, CAR_MAX_SPEED]).
void car_change_speed(Car& car, float delta);

// Change heading by delta radians.
void car_turn(Car& car, float deltaRad);

// Toggle headlight illumination.
void car_toggle_headlights(Car& car);

// Reset car to initial state.
void car_reset(Car& car);

// Return car's forward direction vector (unit, in XZ plane).
Vec3 car_forward(const Car& car);

// Return car's right direction vector (unit, in XZ plane).
Vec3 car_right(const Car& car);

// Convert a point in local car-space to world-space.
Vec3 car_local_to_world(const Car& car, float localX, float localY, float localForward);
