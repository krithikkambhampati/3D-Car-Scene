#include "fan.h"
#include "shader.h"
#include "math_utils.h"
#include "constants.h"
#include <algorithm>
#include <cmath>

float g_fanSpeed = FAN_DEFAULT_SPEED;

namespace {

constexpr float HALF_PI = PI_F * 0.5f;

void set_part_style(GLuint shader,
                    float r, float g, float b,
                    float shininess)
{
    setVec4(shader, "uColor", r, g, b, 1.f);
    setFloat(shader, "uShininess", shininess);
    setInt(shader, "uUseTexture", 0);
}

void draw_part(GLuint shader,
               const Mesh& mesh,
               const float base[16],
               float tx, float ty, float tz,
               float ry,
               float rz,
               float sx, float sy, float sz,
               float r, float g, float b,
               float shininess)
{
    float T[16], Ry[16], Rz[16], S[16];
    float localRot[16], localRS[16], local[16], model[16];
    mat_translate(T, tx, ty, tz);
    mat_rotY(Ry, ry);
    mat_rotZ(Rz, rz);
    mat_scale(S, sx, sy, sz);
    mat_mul(localRot, Ry, Rz);
    mat_mul(localRS, localRot, S);
    mat_mul(local, T, localRS);
    mat_mul(model, base, local);

    setMat4(shader, "uModel", model);
    set_part_style(shader, r, g, b, shininess);
    mesh_draw(mesh);
}

} // namespace

void fan_meshes_init(FanMeshes& fm) {
    build_box(fm.tower, 0.5f, 0.5f, 0.5f);
    build_box(fm.beam, 0.5f, 0.5f, 0.5f);
    build_cylinder(fm.hub, 0.18f, 0.24f, 22);
    build_box(fm.sail, 0.5f, 0.5f, 0.5f);
    build_cylinder(fm.cap, 0.34f, 0.16f, 18);
}

void fan_update(Fan& fan, float dt) {
    fan.spinSpeed = g_fanSpeed;
    fan.angle += fan.spinSpeed * dt;
}

void fan_increase_speed() {
    g_fanSpeed = std::min(g_fanSpeed + FAN_SPEED_INCR, FAN_MAX_SPEED);
}

void fan_decrease_speed() {
    g_fanSpeed = std::max(g_fanSpeed - FAN_SPEED_INCR, FAN_MIN_SPEED);
}

void fan_draw(const Fan& fan, const Building& b,
              const FanMeshes& fm, GLuint shader)
{
    const float yaw = atan2f(-b.facingZ, b.facingX);
    const float sideX = -b.facingZ;
    const float sideZ =  b.facingX;

    const float roofInset = (std::max(b.width, b.depth) > 6.5f) ? 0.9f : 0.7f;
    const float lateralShift = ((b.styleId % 3) - 1) * 0.42f;
    const float anchorX = b.posX + b.facingX * roofInset + sideX * lateralShift;
    const float anchorZ = b.posZ + b.facingZ * roofInset + sideZ * lateralShift;
    const float anchorY = b.height + 0.14f;

    float baseT[16], baseR[16], base[16];
    mat_translate(baseT, anchorX, anchorY, anchorZ);
    mat_rotY(baseR, yaw);
    mat_mul(base, baseT, baseR);

    const float towerHeight = 2.45f + 0.12f * (float)(b.styleId % 3);
    const float legSpread = 0.48f;
    const float legWidth = 0.12f;
    const float towerTopY = towerHeight + 0.12f;

    draw_part(shader, fm.cap, base,
              0.f, 0.12f, 0.f,
              0.f, 0.f,
              1.85f, 0.30f, 1.60f,
              0.24f, 0.25f, 0.28f, 12.f);

    for (float sx : {-legSpread, legSpread}) {
        for (float sz : {-legSpread, legSpread}) {
            draw_part(shader, fm.tower, base,
                      sx, towerHeight * 0.5f, sz,
                      0.f, 0.f,
                      legWidth, towerHeight, legWidth,
                      0.72f, 0.67f, 0.58f, 14.f);
        }
    }

    for (int tier = 0; tier < 2; tier++) {
        float y = 0.82f + tier * 0.88f;
        draw_part(shader, fm.beam, base,
                  0.f, y,  legSpread,
                  0.f,  0.40f,
                  1.18f, 0.06f, 0.06f,
                  0.64f, 0.59f, 0.50f, 10.f);
        draw_part(shader, fm.beam, base,
                  0.f, y, -legSpread,
                  0.f, -0.40f,
                  1.18f, 0.06f, 0.06f,
                  0.64f, 0.59f, 0.50f, 10.f);
        draw_part(shader, fm.beam, base,
                  -legSpread, y, 0.f,
                  HALF_PI,  0.40f,
                  1.18f, 0.06f, 0.06f,
                  0.64f, 0.59f, 0.50f, 10.f);
        draw_part(shader, fm.beam, base,
                   legSpread, y, 0.f,
                  HALF_PI, -0.40f,
                  1.18f, 0.06f, 0.06f,
                  0.64f, 0.59f, 0.50f, 10.f);
    }

    draw_part(shader, fm.cap, base,
              0.f, towerTopY, 0.f,
              0.f, 0.f,
              0.90f, 0.22f, 0.90f,
              0.48f, 0.44f, 0.36f, 18.f);

    draw_part(shader, fm.beam, base,
              0.38f, towerTopY + 0.28f, 0.f,
              0.f, 0.f,
              1.05f, 0.34f, 0.34f,
              0.76f, 0.75f, 0.72f, 26.f);

    draw_part(shader, fm.beam, base,
              -0.42f, towerTopY + 0.24f, 0.f,
              0.f, 0.f,
              0.72f, 0.08f, 0.08f,
              0.64f, 0.63f, 0.61f, 20.f);

    draw_part(shader, fm.sail, base,
              -0.90f, towerTopY + 0.36f, 0.f,
              0.f, 0.f,
              0.20f, 0.54f, 0.82f,
              0.82f, 0.28f, 0.22f, 18.f);

    float hubT[16], hubR[16], hubLocal[16], hubModel[16];
    mat_translate(hubT, 1.00f, towerTopY + 0.28f, 0.f);
    mat_rotZ(hubR, HALF_PI);
    mat_mul(hubLocal, hubT, hubR);
    mat_mul(hubModel, base, hubLocal);
    setMat4(shader, "uModel", hubModel);
    set_part_style(shader, 0.66f, 0.67f, 0.70f, 40.f);
    mesh_draw(fm.hub);

    const float bladeBaseX = 1.18f;
    const float rotorY = towerTopY + 0.28f;
    const float spokeLen = 1.10f;
    const float sailLen = 1.18f;

    for (int k = 0; k < FAN_BLADES; k++) {
        float bladeAngle = fan.angle + k * (2.f * PI_F / FAN_BLADES);

        float spinX[16], bladeBase[16];
        mat_rotX(spinX, bladeAngle);
        mat_translate(hubT, bladeBaseX, rotorY, 0.f);
        mat_mul(bladeBase, hubT, spinX);

        float bladeRoot[16];
        mat_mul(bladeRoot, base, bladeBase);

        draw_part(shader, fm.beam, bladeRoot,
                  0.f, spokeLen * 0.52f, 0.f,
                  0.f, 0.12f,
                  0.10f, spokeLen, 0.08f,
                  0.63f, 0.47f, 0.28f, 14.f);

        draw_part(shader, fm.beam, bladeRoot,
                  0.f, spokeLen * 0.32f, 0.14f,
                  0.f, 1.5707963f,
                  0.05f, 0.38f, 0.05f,
                  0.68f, 0.53f, 0.34f, 10.f);

        draw_part(shader, fm.beam, bladeRoot,
                  0.f, spokeLen * 0.70f, -0.14f,
                  0.f, 1.5707963f,
                  0.05f, 0.40f, 0.05f,
                  0.68f, 0.53f, 0.34f, 10.f);

        draw_part(shader, fm.sail, bladeRoot,
                  0.f, sailLen, 0.07f,
                  0.f, 0.08f,
                  0.07f, 0.72f, 0.46f,
                  0.90f, 0.88f, 0.76f, 8.f);
    }
}
