#pragma once
#include "math_utils.h"
#include "car.h"
#include "light_source.h"
// ============================================================
//  camera_system.h  –  5 switchable camera modes.
// ============================================================

enum class CameraMode {
    SKY          = 1,  // top-down: key '1'
    CAR          = 2,  // front-of-car/roof: key '2'
    GROUND       = 3,  // fixed view from building front: key '3'
    LIGHTSOURCE  = 4,  // follows spotlight[0]: key '4'
    HELICOPTER   = 5,  // chase cam behind car: key '5'
};

struct CameraSystem {
    CameraMode mode     = CameraMode::SKY;
    float groundYaw     = 0.f; // ground-camera look-left/right offset (radians), ±PI/6
    bool freeLookActive = false;
    bool dragActive = false;
    double lastMouseX = 0.0;
    double lastMouseY = 0.0;
    float orbitYaw = -PI_F * 0.5f;
    float orbitPitch = -0.55f;
    float orbitDistance = 45.f;
    Vec3 orbitTarget = {0.f, 4.f, 0.f};
};

// Switch camera mode (1-5)
void camera_set_mode(CameraSystem& cam, int modeNum);

// Adjust ground view yaw (q=left, e=right)
void camera_ground_yaw(CameraSystem& cam, float delta);

void camera_begin_free_look(CameraSystem& cam,
                            const Car& car,
                            const SpotLight* spotlights,
                            double mouseX,
                            double mouseY);

void camera_end_free_look_drag(CameraSystem& cam);

void camera_update_free_look_drag(CameraSystem& cam, double mouseX, double mouseY);

void camera_zoom_free_look(CameraSystem& cam, float delta);

// Compute the view matrix for the current mode.
// car, spotlight[0] used for modes 2,4,5
void camera_compute_view(const CameraSystem& cam,
                         const Car& car,
                         const SpotLight* spotlights,
                         float outView[16]);
