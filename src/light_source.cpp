#include "light_source.h"
#include "shader.h"
#include "math_utils.h"
#include "constants.h"
#include <cmath>

namespace {

static const float LIGHT_COLORS[5][3] = {
    {1.00f, 0.40f, 0.18f},
    {0.16f, 0.80f, 1.00f},
    {1.00f, 0.84f, 0.16f},
    {0.42f, 1.00f, 0.38f},
    {1.00f, 0.26f, 0.76f},
};

void set_metal(GLuint shader,
               float r, float g, float b,
               float shininess)
{
    setVec4(shader, "uColor", r, g, b, 1.f);
    setFloat(shader, "uShininess", shininess);
    setInt(shader, "uUseTexture", 0);
}

void draw_part(GLuint shader,
               const Mesh& mesh,
               float tx, float ty, float tz,
               float ry, float rz,
               float sx, float sy, float sz,
               float r, float g, float b,
               float shininess)
{
    float T[16], Ry[16], Rz[16], S[16];
    float localRot[16], localRS[16], model[16];
    mat_translate(T, tx, ty, tz);
    mat_rotY(Ry, ry);
    mat_rotZ(Rz, rz);
    mat_scale(S, sx, sy, sz);
    mat_mul(localRot, Ry, Rz);
    mat_mul(localRS, localRot, S);
    mat_mul(model, T, localRS);
    setMat4(shader, "uModel", model);
    set_metal(shader, r, g, b, shininess);
    mesh_draw(mesh);
}

Vec3 horizontal_forward(float yaw) {
    return {cosf(yaw), 0.f, -sinf(yaw)};
}

} // namespace

float SpotLight::yaw() const {
    return baseAngle + swingAngle;
}

float SpotLight::pivotY() const {
    return posY + mountHeight;
}

float SpotLight::headX() const {
    Vec3 fwd = horizontal_forward(yaw());
    return posX + fwd.x * headOffset;
}

float SpotLight::headY() const {
    return pivotY() - 0.18f;
}

float SpotLight::headZ() const {
    Vec3 fwd = horizontal_forward(yaw());
    return posZ + fwd.z * headOffset;
}

Vec3 SpotLight::direction() const {
    return Vec3(targetX - headX(), targetY - headY(), targetZ - headZ()).normalized();
}

void spotlights_init(SpotLight* lights, int count,
                     const Building* buildings, int nBuildings,
                     SpotMeshes& meshes)
{
    build_sphere(meshes.sphere, 0.34f, 10, 16);
    build_cylinder(meshes.ring, 0.72f, 0.06f, 22);
    build_box(meshes.arm, 0.5f, 0.5f, 0.5f);
    build_cylinder(meshes.base, 0.34f, 0.5f, 20);
    build_cylinder(meshes.housing, 0.34f, 0.52f, 18);

    for (int i = 0; i < count && i < nBuildings; i++) {
        SpotLight& sl = lights[i];
        const Building& b = buildings[i];

        const bool frontOnX = fabsf(b.facingX) > fabsf(b.facingZ);
        const float sideX = -b.facingZ;
        const float sideZ =  b.facingX;
        const float roofOffset = frontOnX ? b.width * 0.34f : b.depth * 0.34f;
        const float sideShift = ((i % 2 == 0) ? -1.f : 1.f) * (frontOnX ? b.depth : b.width) * 0.10f;

        sl.posX = b.posX + b.facingX * roofOffset + sideX * sideShift;
        sl.posY = b.height + 0.12f;
        sl.posZ = b.posZ + b.facingZ * roofOffset + sideZ * sideShift;

        sl.baseAngle = atan2f(-b.posZ, -b.posX);
        sl.swingAngle = 0.f;
        sl.phase = i * (2.f * PI_F / (float)count);
        sl.headOffset = 2.05f + 0.12f * (float)(i % 3);
        sl.mountHeight = 0.84f + 0.05f * (float)(b.styleId % 2);

        float th = atan2f(-sl.posZ, -sl.posX);
        sl.targetX = TRACK_A * cosf(th);
        sl.targetY = TRACK_GROUND_Y - 0.10f;
        sl.targetZ = TRACK_B * sinf(th);

        sl.colorR = LIGHT_COLORS[i % 5][0];
        sl.colorG = LIGHT_COLORS[i % 5][1];
        sl.colorB = LIGHT_COLORS[i % 5][2];
    }
}

Vec3 spotlight_world_pos(const SpotLight& light) {
    return {light.headX(), light.headY(), light.headZ()};
}

void spotlight_update(SpotLight& light, float dt) {
    light.phase += LIGHT_SWING_SPEED * dt;
    light.swingAngle = LIGHT_SWING_MAX * sinf(light.phase);
}

void spotlight_draw_gimbal(const SpotLight& light,
                           const SpotMeshes& meshes,
                           GLuint shader)
{
    const float yaw = light.yaw();
    const Vec3 dir = light.direction();
    const float pitch = asinf(dir.y);
    const float pivotY = light.pivotY();

    draw_part(shader, meshes.arm,
              light.posX, light.posY + 0.08f, light.posZ,
              0.f, 0.f,
              1.65f, 0.16f, 1.40f,
              0.20f, 0.21f, 0.24f, 10.f);

    draw_part(shader, meshes.base,
              light.posX, light.posY + 0.30f, light.posZ,
              0.f, 0.f,
              0.88f, 0.68f, 0.88f,
              0.30f, 0.31f, 0.35f, 18.f);

    draw_part(shader, meshes.base,
              light.posX, light.posY + 0.78f, light.posZ,
              0.f, 0.f,
              0.40f, light.mountHeight, 0.40f,
              0.56f, 0.58f, 0.62f, 24.f);

    draw_part(shader, meshes.ring,
              light.posX, pivotY + 0.06f, light.posZ,
              yaw, 0.f,
              0.92f, 1.0f, 0.92f,
              0.70f, 0.72f, 0.76f, 30.f);

    draw_part(shader, meshes.base,
              light.posX, pivotY + 0.10f, light.posZ,
              yaw, 0.f,
              0.60f, 0.20f, 0.60f,
              0.46f, 0.48f, 0.54f, 22.f);

    const Vec3 fwd = horizontal_forward(yaw);
    const Vec3 side(-fwd.z, 0.f, fwd.x);
    const float sideArm = 0.48f;
    const float braceDrop = 0.54f;
    const float braceMid = light.headOffset * 0.55f;

    for (float s : {-1.f, 1.f}) {
        draw_part(shader, meshes.arm,
                  light.posX + side.x * sideArm * s,
                  pivotY - braceDrop * 0.5f,
                  light.posZ + side.z * sideArm * s,
                  yaw, 0.f,
                  0.10f, braceDrop, 0.10f,
                  0.74f, 0.76f, 0.80f, 28.f);

        draw_part(shader, meshes.arm,
                  light.posX + fwd.x * braceMid + side.x * sideArm * s,
                  pivotY - braceDrop * 0.15f,
                  light.posZ + fwd.z * braceMid + side.z * sideArm * s,
                  yaw, s > 0.f ? -0.22f : 0.22f,
                  0.10f, light.headOffset * 0.92f, 0.10f,
                  0.66f, 0.68f, 0.72f, 24.f);
    }

    draw_part(shader, meshes.arm,
              light.posX + fwd.x * braceMid,
              pivotY + 0.12f,
              light.posZ + fwd.z * braceMid,
              yaw, 0.f,
              light.headOffset * 0.88f, 0.10f, 1.06f,
              0.56f, 0.58f, 0.62f, 22.f);

    draw_part(shader, meshes.base,
              light.headX(), light.headY(), light.headZ(),
              yaw, pitch,
              0.32f, 0.24f, 0.32f,
              0.82f, 0.84f, 0.88f, 34.f);

    draw_part(shader, meshes.housing,
              light.headX() - dir.x * 0.18f,
              light.headY() - dir.y * 0.18f,
              light.headZ() - dir.z * 0.18f,
              yaw, pitch,
              0.64f, 0.90f, 0.64f,
              0.17f, 0.18f, 0.21f, 34.f);

    draw_part(shader, meshes.arm,
              light.headX() - dir.x * 0.56f,
              light.headY() - dir.y * 0.56f + 0.10f,
              light.headZ() - dir.z * 0.56f,
              yaw, pitch - 0.10f,
              0.48f, 0.18f, 0.84f,
              0.28f, 0.30f, 0.34f, 18.f);
}

void spotlight_draw_marker(const SpotLight& light,
                           const SpotMeshes& meshes,
                           GLuint emissiveShader)
{
    float T[16], S[16], tmp[16], model[16];
    mat_translate(T, light.headX(), light.headY(), light.headZ());
    mat_scale(S, 0.85f, 0.85f, 0.85f);
    mat_mul(tmp, T, S);
    mat_identity(model);
    mat_mul(model, tmp, model);

    setMat4(emissiveShader, "uModel", model);
    setVec4(emissiveShader, "uColor",
            light.colorR * 1.72f,
            light.colorG * 1.72f,
            light.colorB * 1.72f,
            1.f);
    mesh_draw(meshes.sphere);
}
