#include "camera_system.h"
#include "constants.h"
#include <cmath>
#include <algorithm>

struct CameraPose {
    Vec3 eye;
    Vec3 center;
    Vec3 up;
};

static CameraPose preset_pose(const CameraSystem& cam,
                              const Car& car,
                              const SpotLight* spotlights)
{
    CameraPose pose;

    switch (cam.mode) {
    case CameraMode::SKY:
        pose.eye    = {0.f, 90.f, 0.001f};
        pose.center = {0.f, 0.f, 0.f};
        pose.up     = {0.f, 0.f, -1.f};
        break;

    case CameraMode::CAR: {
        Vec3 fwd = car_forward(car);
        Vec3 carPos = {car.posX, 0.f, car.posZ};
        pose.eye    = carPos + fwd * 0.65f + Vec3(0.f, 2.0f, 0.f);
        pose.center = pose.eye + fwd * 30.f + Vec3(0.f, -0.35f, 0.f);
        pose.up     = {0.f, 1.f, 0.f};
        break;
    }

    case CameraMode::GROUND: {
        Vec3 fixedPos = {-ARENA_HALF_W + 8.f, 7.0f, 12.f};
        float baseAngle = 0.16f;
        float lookAngle = baseAngle + cam.groundYaw;
        Vec3 lookDir = { cosf(lookAngle), -0.10f, sinf(lookAngle) };
        pose.eye    = fixedPos;
        pose.center = fixedPos + lookDir * 40.f;
        pose.up     = {0.f, 1.f, 0.f};
        break;
    }

    case CameraMode::LIGHTSOURCE: {
        Vec3 lpos = spotlight_world_pos(spotlights[0]);
        Vec3 dir  = spotlights[0].direction();
        pose.eye    = lpos + Vec3(0.f, 1.25f, 0.f) - dir * 0.20f;
        pose.center = lpos + dir * 28.f + Vec3(0.f, 0.35f, 0.f);
        pose.up     = {0.f, 1.f, 0.f};
        break;
    }

    case CameraMode::HELICOPTER: {
        Vec3 fwd = car_forward(car);
        Vec3 carPos = {car.posX, 0.f, car.posZ};
        pose.eye    = carPos - fwd * 12.f + Vec3(0.f, 8.5f, 0.f);
        pose.center = carPos + fwd * 3.0f + Vec3(0.f, 1.6f, 0.f);
        pose.up     = {0.f, 1.f, 0.f};
        break;
    }
    }

    return pose;
}

static float clamp_pitch(float pitch) {
    return std::clamp(pitch, -1.35f, 1.25f);
}

void camera_set_mode(CameraSystem& cam, int modeNum) {
    switch (modeNum) {
        case 1: cam.mode = CameraMode::SKY;        break;
        case 2: cam.mode = CameraMode::CAR;        break;
        case 3: cam.mode = CameraMode::GROUND;     break;
        case 4: cam.mode = CameraMode::LIGHTSOURCE;break;
        case 5: cam.mode = CameraMode::HELICOPTER; break;
    }
    cam.freeLookActive = false;
    cam.dragActive = false;
}

void camera_ground_yaw(CameraSystem& cam, float delta) {
    cam.groundYaw += delta;
    const float MAX_YAW = PI_F / 6.f; // 30 degrees
    if (cam.groundYaw >  MAX_YAW) cam.groundYaw =  MAX_YAW;
    if (cam.groundYaw < -MAX_YAW) cam.groundYaw = -MAX_YAW;
}

void camera_begin_free_look(CameraSystem& cam,
                            const Car& car,
                            const SpotLight* spotlights,
                            double mouseX,
                            double mouseY)
{
    if (cam.freeLookActive) {
        cam.dragActive = true;
        cam.lastMouseX = mouseX;
        cam.lastMouseY = mouseY;
        return;
    }

    CameraPose pose = preset_pose(cam, car, spotlights);
    Vec3 offset = pose.eye - pose.center;
    float dist = std::max(offset.length(), 4.0f);
    Vec3 dir = offset.normalized();

    cam.freeLookActive = true;
    cam.dragActive = true;
    cam.lastMouseX = mouseX;
    cam.lastMouseY = mouseY;
    cam.orbitTarget = pose.center;
    cam.orbitDistance = dist;
    cam.orbitYaw = atan2f(dir.z, dir.x);
    cam.orbitPitch = clamp_pitch(asinf(dir.y));
}

void camera_end_free_look_drag(CameraSystem& cam) {
    cam.dragActive = false;
}

void camera_update_free_look_drag(CameraSystem& cam, double mouseX, double mouseY) {
    if (!cam.dragActive) return;
    float dx = (float)(mouseX - cam.lastMouseX);
    float dy = (float)(mouseY - cam.lastMouseY);
    cam.lastMouseX = mouseX;
    cam.lastMouseY = mouseY;

    const float sensitivity = 0.0065f;
    cam.orbitYaw   -= dx * sensitivity;
    cam.orbitPitch  = clamp_pitch(cam.orbitPitch - dy * sensitivity);
}

void camera_zoom_free_look(CameraSystem& cam, float delta) {
    if (!cam.freeLookActive) return;
    cam.orbitDistance = std::clamp(cam.orbitDistance - delta * 2.0f, 6.0f, 150.0f);
}

void camera_compute_view(const CameraSystem& cam,
                         const Car& car,
                         const SpotLight* spotlights,
                         float outView[16])
{
    CameraPose pose = preset_pose(cam, car, spotlights);

    if (cam.freeLookActive) {
        Vec3 orbitDir(cosf(cam.orbitPitch) * cosf(cam.orbitYaw),
                      sinf(cam.orbitPitch),
                      cosf(cam.orbitPitch) * sinf(cam.orbitYaw));
        pose.center = cam.orbitTarget;
        pose.eye = pose.center + orbitDir * cam.orbitDistance;
        pose.up = {0.f, 1.f, 0.f};
    }

    mat_lookAt(outView, pose.eye, pose.center, pose.up);
}
