#include "car.h"
#include "shader.h"
#include <cmath>

Vec3 car_forward(const Car& car) {
    // Keep forward consistent with mat_rotY convention used by rendering.
    return { cosf(car.heading), 0.f, -sinf(car.heading) };
}

Vec3 car_right(const Car& car) {
    Vec3 fwd = car_forward(car);
    return { -fwd.z, 0.f, fwd.x };
}

Vec3 car_local_to_world(const Car& car, float localX, float localY, float localForward) {
    Vec3 origin(car.posX, 0.f, car.posZ);
    return origin + car_right(car) * localX + Vec3(0.f, localY, 0.f) + car_forward(car) * localForward;
}

void car_init(Car& car) {
    build_box    (car.bodyMesh,      CAR_BODY_HW,   CAR_BODY_HH,   CAR_BODY_HL);
    build_box    (car.cabinMesh,     CAR_CABIN_HW,  CAR_CABIN_HH,  CAR_CABIN_HL);
    build_cylinder(car.wheelMesh,    CAR_WHEEL_R,   CAR_WHEEL_W, 20);
    build_box    (car.headlightMesh, 0.18f,  0.12f, 0.08f);
    build_box    (car.headlightBeamMesh, 0.24f, 0.10f, 1.0f);
}

void car_update(Car& car, float dt) {
    Vec3 fwd = car_forward(car);
    car.posX += fwd.x * car.speed * dt;
    car.posZ += fwd.z * car.speed * dt;
}

void car_change_speed(Car& car, float delta) {
    car.speed += delta;
    if (car.speed > CAR_MAX_SPEED) car.speed = CAR_MAX_SPEED;
    if (car.speed < CAR_MIN_SPEED) car.speed = CAR_MIN_SPEED;
}

void car_turn(Car& car, float deltaRad) {
    car.heading += deltaRad;
}

void car_toggle_headlights(Car& car) {
    car.headlightsOn = !car.headlightsOn;
}

void car_reset(Car& car) {
    car.posX    = 0.f;
    car.posZ    = -(TRACK_B - 3.5f);
    car.heading = 0.f;
    car.speed   = 0.f;
    car.stopped = false;
    car.headlightsOn = true;
}

// Helper: draw one part with its own model = parent * local
// (draw_part left for potential future use)

void car_draw(const Car& car, GLuint shader) {
    // Build root transform:
    // physics forward is +X, while our mesh length is along +Z.
    float root[16], T[16], R[16], align[16], tmpRoot[16];
    mat_translate(T, car.posX, 0.f, car.posZ);
    mat_rotY(R, car.heading);
    mat_rotY(align, PI_F * 0.5f);
    mat_mul(tmpRoot, R, align);
    mat_mul(root, T, tmpRoot);

    auto draw_box_part = [&](float lx, float ly, float lz,
                             float sx, float sy, float sz,
                             float cr, float cg, float cb,
                             float shininess,
                             float alpha = 1.f,
                             GLuint tex = 0) {
        float LT[16], LS[16], LM[16], M[16];
        mat_translate(LT, lx, ly, lz);
        mat_scale(LS, sx, sy, sz);
        mat_mul(LM, LT, LS);
        mat_mul(M, root, LM);
        setMat4(shader, "uModel", M);
        setVec4(shader, "uColor", cr, cg, cb, alpha);
        setFloat(shader, "uShininess", shininess);
        setInt(shader, "uUseTexture", tex ? 1 : 0);
        if (tex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            setInt(shader, "uTex", 0);
        }
        mesh_draw(car.bodyMesh);
        if (tex) glBindTexture(GL_TEXTURE_2D, 0);
    };

    // Main shell: lower body + upper greenhouse for a cleaner silhouette.
    draw_box_part(0.f, CAR_BODY_Y + 0.46f, 0.f,
                  1.14f, 0.56f, 2.02f,
                  0.84f, 0.92f, 1.00f, 84.f, 1.f, car.bodyTex);
    draw_box_part(0.f, CAR_BODY_Y + 1.10f, -0.15f,
                  0.86f, 0.38f, 1.05f,
                  0.82f, 0.90f, 1.00f, 78.f, 1.f, car.bodyTex);

    // Hood + rear deck.
    draw_box_part(0.f, CAR_BODY_Y + 0.74f, 1.18f,
                  0.96f, 0.22f, 0.40f,
                  0.86f, 0.94f, 1.00f, 78.f, 1.f, car.bodyTex);
    draw_box_part(0.f, CAR_BODY_Y + 0.72f, -1.34f,
                  0.90f, 0.20f, 0.48f,
                  0.80f, 0.88f, 0.98f, 72.f, 1.f, car.bodyTex);

    // Bumpers, rocker skirts, and grille strip.
    draw_box_part(0.f, CAR_BODY_Y + 0.36f, 1.93f,
                  0.92f, 0.20f, 0.12f,
                  0.24f, 0.25f, 0.28f, 28.f);
    draw_box_part(0.f, CAR_BODY_Y + 0.34f, -2.02f,
                  0.90f, 0.18f, 0.14f,
                  0.14f, 0.15f, 0.17f, 22.f);
    draw_box_part( 1.10f, CAR_BODY_Y + 0.30f, -0.10f,
                  0.08f, 0.14f, 1.70f,
                  0.14f, 0.15f, 0.17f, 18.f);
    draw_box_part(-1.10f, CAR_BODY_Y + 0.30f, -0.10f,
                  0.08f, 0.14f, 1.70f,
                  0.14f, 0.15f, 0.17f, 18.f);
    draw_box_part(0.f, CAR_BODY_Y + 0.52f, 1.88f,
                  0.62f, 0.08f, 0.04f,
                  0.30f, 0.32f, 0.34f, 38.f);

    // Dark windows and pillars.
    draw_box_part(0.f, CAR_BODY_Y + 1.14f, 0.30f,
                  0.76f, 0.16f, 0.15f,
                  0.08f, 0.12f, 0.16f, 120.f, 0.92f);
    draw_box_part(0.f, CAR_BODY_Y + 1.14f, -1.02f,
                  0.74f, 0.16f, 0.14f,
                  0.08f, 0.12f, 0.16f, 120.f, 0.92f);
    draw_box_part( 0.90f, CAR_BODY_Y + 1.06f, -0.25f,
                  0.05f, 0.22f, 0.66f,
                  0.07f, 0.11f, 0.16f, 108.f, 0.90f);
    draw_box_part(-0.90f, CAR_BODY_Y + 1.06f, -0.25f,
                  0.05f, 0.22f, 0.66f,
                  0.07f, 0.11f, 0.16f, 108.f, 0.90f);

    // Small mirrors.
    draw_box_part( 1.12f, CAR_BODY_Y + 0.95f, 0.48f,
                  0.10f, 0.06f, 0.10f,
                  0.10f, 0.10f, 0.10f, 26.f);
    draw_box_part(-1.12f, CAR_BODY_Y + 0.95f, 0.48f,
                  0.10f, 0.06f, 0.10f,
                  0.10f, 0.10f, 0.10f, 26.f);

    // Wheel arches.
    const float archY = CAR_WHEEL_R + 0.18f;
    draw_box_part( 1.08f, archY,  1.26f, 0.11f, 0.30f, 0.56f, 0.12f, 0.12f, 0.12f, 18.f);
    draw_box_part(-1.08f, archY,  1.26f, 0.11f, 0.30f, 0.56f, 0.12f, 0.12f, 0.12f, 18.f);
    draw_box_part( 1.08f, archY, -1.24f, 0.11f, 0.30f, 0.56f, 0.12f, 0.12f, 0.12f, 18.f);
    draw_box_part(-1.08f, archY, -1.24f, 0.11f, 0.30f, 0.56f, 0.12f, 0.12f, 0.12f, 18.f);

    // Wheels + rim + center cap.
    struct WheelPos { float x, z; };
    const WheelPos wps[4] = {{-1.28f, 1.24f}, {1.28f, 1.24f}, {-1.28f, -1.24f}, {1.28f, -1.24f}};
    for (const WheelPos& wp : wps) {
        float wT[16], wRz[16], wLocal[16], wModel[16];
        mat_translate(wT, wp.x, CAR_WHEEL_R + 0.02f, wp.z);
        mat_rotZ(wRz, 1.5708f);
        mat_mul(wLocal, wT, wRz);
        mat_mul(wModel, root, wLocal);

        setMat4(shader, "uModel", wModel);
        setVec4(shader, "uColor", 0.10f, 0.10f, 0.11f, 1.f);
        setFloat(shader, "uShininess", 10.f);
        setInt(shader, "uUseTexture", car.rubberTex ? 1 : 0);
        if (car.rubberTex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, car.rubberTex);
            setInt(shader, "uTex", 0);
        }
        mesh_draw(car.wheelMesh);
        if (car.rubberTex) glBindTexture(GL_TEXTURE_2D, 0);

        float rimS[16], rimTmp[16], rimM[16];
        mat_scale(rimS, 0.62f, 1.05f, 0.62f);
        mat_mul(rimTmp, wLocal, rimS);
        mat_mul(rimM, root, rimTmp);
        setMat4(shader, "uModel", rimM);
        setVec4(shader, "uColor", 0.72f, 0.74f, 0.78f, 1.f);
        setFloat(shader, "uShininess", 62.f);
        setInt(shader, "uUseTexture", car.metalTex ? 1 : 0);
        if (car.metalTex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, car.metalTex);
            setInt(shader, "uTex", 0);
        }
        mesh_draw(car.wheelMesh);
        if (car.metalTex) glBindTexture(GL_TEXTURE_2D, 0);

        float capS[16], capTmp[16], capM[16];
        mat_scale(capS, 0.34f, 1.08f, 0.34f);
        mat_mul(capTmp, wLocal, capS);
        mat_mul(capM, root, capTmp);
        setMat4(shader, "uModel", capM);
        setVec4(shader, "uColor", 0.46f, 0.48f, 0.52f, 1.f);
        setFloat(shader, "uShininess", 48.f);
        setInt(shader, "uUseTexture", 0);
        mesh_draw(car.wheelMesh);
    }

    // Headlights.
    const float hly = CAR_BODY_Y + 0.62f;
    draw_box_part(-0.54f, hly, 1.96f,
                  0.18f, 0.10f, 0.06f,
                  car.headlightsOn ? 1.00f : 0.42f,
                  car.headlightsOn ? 0.97f : 0.42f,
                  car.headlightsOn ? 0.82f : 0.45f,
                  car.headlightsOn ? 56.f : 10.f);
    draw_box_part( 0.54f, hly, 1.96f,
                  0.18f, 0.10f, 0.06f,
                  car.headlightsOn ? 1.00f : 0.42f,
                  car.headlightsOn ? 0.97f : 0.42f,
                  car.headlightsOn ? 0.82f : 0.45f,
                  car.headlightsOn ? 56.f : 10.f);

    // Tail lights and plate band.
    draw_box_part(-0.52f, CAR_BODY_Y + 0.58f, -2.02f,
                  0.18f, 0.09f, 0.05f,
                  0.95f, 0.16f, 0.15f, 52.f);
    draw_box_part( 0.52f, CAR_BODY_Y + 0.58f, -2.02f,
                  0.18f, 0.09f, 0.05f,
                  0.95f, 0.16f, 0.15f, 52.f);
    draw_box_part(0.f, CAR_BODY_Y + 0.46f, -2.01f,
                  0.32f, 0.07f, 0.04f,
                  0.20f, 0.20f, 0.22f, 20.f);
}
