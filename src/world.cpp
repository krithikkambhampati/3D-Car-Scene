#include "world.h"
#include "shader.h"
#include "texture.h"
#include "math_utils.h"
#include "collision.h"
#include "constants.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

// Night sky key light (moon-like, low intensity)
static const Vec3 SUN_POS = { 24.f, 42.f, -14.f };

static const StreetLight STREET_LAYOUT[4] = {
    { -24.f,  24.f, 8.2f, 1.00f, 0.95f, 0.86f, 1.10f },
    {  24.f,  24.f, 8.2f, 1.00f, 0.95f, 0.86f, 1.10f },
    { -24.f, -24.f, 8.2f, 1.00f, 0.95f, 0.86f, 1.10f },
    {  24.f, -24.f, 8.2f, 1.00f, 0.95f, 0.86f, 1.10f },
};

static const Tree TREE_LAYOUT[6] = {
    { -47.f,  27.f, 4.6f, 2.7f, 2.05f },
    {   0.f,  31.f, 5.0f, 2.9f, 2.18f },
    {  47.f,  27.f, 4.6f, 2.7f, 2.05f },
    { -47.f, -31.f, 4.8f, 2.8f, 2.10f },
    {   0.f, -33.f, 5.2f, 3.1f, 2.25f },
    {  47.f, -31.f, 4.8f, 2.8f, 2.10f },
};

static float clamp01(float v) {
    return std::max(0.f, std::min(1.f, v));
}

static float hash01(int n) {
    float x = sinf((float)n * 12.9898f + 78.233f) * 43758.5453f;
    return x - floorf(x);
}

static CarLight headlight_state(const Car& car, float sideOffset) {
    const float forwardOffset = CAR_BODY_HL + 1.10f;
    const float verticalOffset = CAR_BODY_Y + CAR_BODY_HH - 0.06f;
    CarLight light;
    light.position = car_local_to_world(car, sideOffset, verticalOffset, forwardOffset);
    light.color = {1.70f, 1.62f, 1.42f};
    return light;
}

static Rect2D street_light_rect(const StreetLight& s) {
    const float r = s.collisionRadius;
    return { s.x - r, s.x + r, s.z - r, s.z + r };
}

static Vec3 street_lamp_dir_xz(const StreetLight& s) {
    Vec3 toCenter(-s.x, 0.f, -s.z);
    if (toCenter.length() < 1e-4f)
        return {1.f, 0.f, 0.f};
    return toCenter.normalized();
}

static Vec3 street_lamp_pos(const StreetLight& s) {
    Vec3 d = street_lamp_dir_xz(s);
    const float reach = 1.20f;
    return { s.x + d.x * reach, s.height - 0.85f, s.z + d.z * reach };
}

static bool circle_circle_overlap(float ax, float az, float ar,
                                  float bx, float bz, float br,
                                  float eps = 0.1f)
{
    float dx = ax - bx;
    float dz = az - bz;
    float reach = ar + br + eps;
    return dx * dx + dz * dz < reach * reach;
}

static void set_textured_style(GLuint shader,
                               GLuint texture,
                               float tintR, float tintG, float tintB,
                               float shininess)
{
    setVec4(shader, "uColor", tintR, tintG, tintB, 1.f);
    setFloat(shader, "uShininess", shininess);
    setInt(shader, "uUseTexture", texture ? 1 : 0);
    if (texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        setInt(shader, "uTex", 0);
    }
}

static void trigger_bullet_time(World& w) {
    w.bulletTimeRemaining = BULLET_TIME_DURATION;
    w.postCrashHold = 0.4f;
}

static CarReplayState car_to_state(const Car& c) {
    CarReplayState s;
    s.x = c.posX; s.z = c.posZ; s.heading = c.heading; s.speed = c.speed;
    return s;
}

static void apply_state_to_car(Car& c, const CarReplayState& s) {
    c.posX = s.x;
    c.posZ = s.z;
    c.heading = s.heading;
    c.speed = s.speed;
}

static void push_history(World& w, const CarReplayState& s) {
    w.history[w.historyHead] = s;
    w.historyHead = (w.historyHead + 1) % World::REPLAY_BUFFER_CAP;
    if (w.historyCount < World::REPLAY_BUFFER_CAP) w.historyCount++;
}

static void start_collision_replay(World& w) {
    w.replayFrameCount = std::min(w.historyCount, 220);
    int start = (w.historyHead - w.replayFrameCount + World::REPLAY_BUFFER_CAP) % World::REPLAY_BUFFER_CAP;
    for (int i = 0; i < w.replayFrameCount; i++) {
        int idx = (start + i) % World::REPLAY_BUFFER_CAP;
        w.replayFrames[i] = w.history[idx];
    }
    if (w.replayFrameCount < World::REPLAY_BUFFER_CAP) {
        w.replayFrames[w.replayFrameCount++] = w.crashState;
    }

    w.replayElapsed = 0.f;
    w.replayActive = (w.replayFrameCount >= 2);
    if (w.replayActive) {
        // Slow-motion replay takes longer than real duration of captured frames.
        w.replayDuration = std::max(2.8f, (float)w.replayFrameCount / 42.0f * 2.9f);
    }
}

void world_init(World& w) {
    // Compile shaders
    w.mainShader     = createShaderProgram("shaders/vertex.glsl",
                                           "shaders/fragment.glsl");
    w.emissiveShader = createShaderProgram("shaders/emissive_vert.glsl",
                                           "shaders/emissive_frag.glsl");
    if (!w.mainShader || !w.emissiveShader)
        std::cerr << "[world] Shader compilation failed!\n";

    // Generate procedural textures
    w.brickTex   = gen_brick_texture();
    w.woodTex    = gen_wood_texture();
    w.stoneTex   = gen_stone_texture();
    w.plasterTex = gen_plaster_texture();
    w.roadTex    = gen_road_texture();
    w.grassTex   = gen_grass_texture();
    w.barkTex    = gen_bark_texture();
    w.leafTex    = gen_leaf_texture();
    w.carTex     = gen_car_paint_texture();
    w.carMetalTex = gen_car_metal_texture();
    w.carRubberTex = gen_car_rubber_texture();

    // Build scene
    car_init(w.car);
    // Use the provided blue metallic paint texture for car body.
    w.car.bodyTex = w.carTex;
    w.car.metalTex = w.carMetalTex;
    w.car.rubberTex = w.carRubberTex;
    track_init(w.track, w.roadTex, w.grassTex);
    buildings_init(w.buildings, NUM_BUILDINGS, w.buildingMeshes,
                   w.brickTex, w.woodTex, w.stoneTex, w.plasterTex);
    fan_meshes_init(w.fanMeshes);
    for (int i = 0; i < NUM_BUILDINGS; i++)
        w.fans[i].spinSpeed = g_fanSpeed;
    spotlights_init(w.spotlights, NUM_BUILDINGS, w.buildings, NUM_BUILDINGS, w.spotMeshes);
    wall_init(w.wall);

    build_cylinder(w.streetPoleMesh, 0.20f, 0.5f, 18);
    build_sphere(w.streetLampMesh, 0.46f, 10, 16);
    build_cylinder(w.treeTrunkMesh, 0.26f, 0.5f, 22);
    build_sphere(w.treeCanopyMesh, 0.95f, 16, 24);
    for (int i = 0; i < w.numStreetLights; i++)
        w.streetLights[i] = STREET_LAYOUT[i];
    for (int i = 0; i < w.numTrees; i++)
        w.trees[i] = TREE_LAYOUT[i];

    w.historyHead = 0;
    w.historyCount = 0;
    w.replayFrameCount = 0;
    w.replayActive = false;
    w.replayElapsed = 0.f;
    w.replayDuration = 3.4f;
    w.postCrashHold = 0.f;
    w.streetLightsEnabled = true;
    w.sceneTime = 0.f;

    w.stormEnabled = false;
    w.stormBlend = 0.f;
    w.stormRainScroll = 0.f;
    w.lightningCooldown = 1.2f;
    w.lightningFlash = 0.f;
    w.lightningStrikeX = 0.f;
    w.lightningStrikeZ = 0.f;
    w.lightningSeed = 0;
    w.streetLightFlicker = 1.f;
    w.carVelX = 0.f;
    w.carVelZ = 0.f;
    w.stormSkidPhase = 0.f;
}

void world_update(World& w, float dt) {
    w.sceneTime += dt;

    // Storm progression
    {
        float target = w.stormEnabled ? 1.f : 0.f;
        float blendRate = w.stormEnabled ? 1.35f : 1.65f;
        w.stormBlend += (target - w.stormBlend) * std::min(1.f, dt * blendRate * 2.0f);
        w.stormBlend = clamp01(w.stormBlend);
        w.stormRainScroll += dt * (0.45f + 0.95f * w.stormBlend);

        if (w.stormBlend > 0.02f) {
            w.lightningCooldown -= dt * (0.8f + 0.7f * w.stormBlend);
            if (w.lightningCooldown <= 0.f) {
                w.lightningSeed++;
                float rx = hash01(w.lightningSeed * 17 + 3) * 2.f - 1.f;
                float rz = hash01(w.lightningSeed * 23 + 9) * 2.f - 1.f;
                w.lightningStrikeX = rx * (ARENA_HALF_W - 6.f);
                w.lightningStrikeZ = rz * (ARENA_HALF_D - 6.f);
                w.lightningFlash = 1.f;

                float nextGap = STORM_LIGHTNING_MIN_GAP +
                                hash01(w.lightningSeed * 31 + 11) *
                                (STORM_LIGHTNING_MAX_GAP - STORM_LIGHTNING_MIN_GAP);
                w.lightningCooldown = nextGap;
            }

            w.lightningFlash = std::max(0.f, w.lightningFlash - dt * 2.9f);

            int flickBucket = (int)(w.sceneTime * 38.f);
            float bucketNoise = hash01(flickBucket * 13 + 21);
            float dip = (bucketNoise < (0.08f + 0.18f * w.stormBlend))
                      ? (0.12f + 0.33f * hash01(flickBucket * 7 + 5))
                      : 1.f;
            float wave = 0.80f + 0.20f * sinf(w.sceneTime * 34.f + 3.7f);
            w.streetLightFlicker = clamp01(wave * dip);
            w.streetLightFlicker = std::max(0.05f, w.streetLightFlicker);
        } else {
            w.lightningFlash = std::max(0.f, w.lightningFlash - dt * 3.2f);
            w.streetLightFlicker = 1.f;
            if (w.lightningCooldown < 0.8f) w.lightningCooldown = 0.8f;
        }
    }

    // User reset (Backspace) flips car.stopped=false; clear replay then.
    if (w.replayActive && !w.car.stopped) {
        w.replayActive = false;
        w.replayFrameCount = 0;
        w.replayElapsed = 0.f;
        w.bulletTimeRemaining = 0.f;
        w.postCrashHold = 0.f;
        w.historyHead = 0;
        w.historyCount = 0;
        w.carVelX = 0.f;
        w.carVelZ = 0.f;
    }

    float simDt = dt;
    if (w.bulletTimeRemaining > 0.f) {
        float t = w.bulletTimeRemaining / BULLET_TIME_DURATION;
        float replaySlow = BULLET_TIME_SCALE + (1.f - BULLET_TIME_SCALE) * (1.f - t * 0.8f);
        simDt = dt * replaySlow;
        w.bulletTimeRemaining = std::max(0.f, w.bulletTimeRemaining - dt);
    }

    // Update fans + spotlights.
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        fan_update(w.fans[i], simDt);
        spotlight_update(w.spotlights[i], simDt);
    }

    // During replay, drive the car transform from recorded states.
    if (w.replayActive) {
        if (w.postCrashHold > 0.f) {
            w.postCrashHold = std::max(0.f, w.postCrashHold - dt);
            apply_state_to_car(w.car, w.crashState);
            w.car.speed = 0.f;
            w.carVelX = 0.f;
            w.carVelZ = 0.f;
            return;
        }

        w.replayElapsed += dt;
        float t = (w.replayDuration > 0.f) ? (w.replayElapsed / w.replayDuration) : 1.f;
        if (t > 1.f) t = 1.f;

        float framePos = t * (float)(w.replayFrameCount - 1);
        int i0 = (int)framePos;
        int i1 = std::min(i0 + 1, w.replayFrameCount - 1);
        float a = framePos - (float)i0;

        const CarReplayState& s0 = w.replayFrames[i0];
        const CarReplayState& s1 = w.replayFrames[i1];
        CarReplayState s;
        s.x = s0.x + (s1.x - s0.x) * a;
        s.z = s0.z + (s1.z - s0.z) * a;
        s.heading = s0.heading + (s1.heading - s0.heading) * a;
        s.speed = 0.f;
        apply_state_to_car(w.car, s);

        if (t >= 1.f) {
            w.replayActive = false;
            apply_state_to_car(w.car, w.crashState);
            w.car.speed = 0.f;
            w.car.stopped = true;
            w.historyHead = 0;
            w.historyCount = 0;
            w.carVelX = 0.f;
            w.carVelZ = 0.f;
        }
        return;
    }

    // Normal car motion + collision detection.
    if (!w.car.stopped) {
        push_history(w, car_to_state(w.car));

        const float wet = clamp01(w.stormBlend);
        if (wet > 0.01f) {
            Vec3 fwd = car_forward(w.car);
            Vec3 right = car_right(w.car);

            float targetVX = fwd.x * w.car.speed;
            float targetVZ = fwd.z * w.car.speed;
            float align = STORM_SLIP_ALIGN_CLEAR * (1.f - wet) + STORM_SLIP_ALIGN_WET * wet;
            float follow = std::min(1.f, align * simDt);

            w.carVelX += (targetVX - w.carVelX) * follow;
            w.carVelZ += (targetVZ - w.carVelZ) * follow;

            float speedNorm = std::min(1.f, fabsf(w.car.speed) / CAR_MAX_SPEED);
            w.stormSkidPhase += simDt * (8.f + 10.f * speedNorm);
            float skidWave = sinf(w.stormSkidPhase) + 0.45f * sinf(w.stormSkidPhase * 2.31f + 0.9f);
            float lateral = skidWave * wet * speedNorm * 3.2f;

            w.carVelX += right.x * lateral * simDt;
            w.carVelZ += right.z * lateral * simDt;

            float drag = std::max(0.f, 1.f - simDt * (0.18f + 0.15f * (1.f - wet)));
            w.carVelX *= drag;
            w.carVelZ *= drag;

            w.car.posX += w.carVelX * simDt;
            w.car.posZ += w.carVelZ * simDt;
        } else {
            car_update(w.car, simDt);
            Vec3 fwd = car_forward(w.car);
            w.carVelX = fwd.x * w.car.speed;
            w.carVelZ = fwd.z * w.car.speed;
            w.stormSkidPhase = 0.f;
        }

        bool crashed = false;
        if (outside_arena(w.car.posX, w.car.posZ, CAR_RADIUS, ARENA_HALF_W, ARENA_HALF_D)) {
            crashed = true;
        }

        for (int i = 0; i < NUM_BUILDINGS && !crashed; i++) {
            if (circle_rect_overlap(w.car.posX, w.car.posZ, CAR_RADIUS,
                                    w.buildings[i].footprint)) {
                crashed = true;
            }
        }

        for (int i = 0; i < w.numStreetLights && !crashed; i++) {
            if (circle_rect_overlap(w.car.posX, w.car.posZ, CAR_RADIUS,
                                    street_light_rect(w.streetLights[i]), 0.15f)) {
                crashed = true;
            }
        }

        for (int i = 0; i < w.numTrees && !crashed; i++) {
            if (circle_circle_overlap(w.car.posX, w.car.posZ, CAR_RADIUS,
                                      w.trees[i].x, w.trees[i].z, w.trees[i].collisionRadius, 0.12f)) {
                crashed = true;
            }
        }

        if (crashed) {
            w.crashState = car_to_state(w.car);
            w.car.stopped = true;
            w.car.speed   = 0.f;
            w.carVelX = 0.f;
            w.carVelZ = 0.f;
            trigger_bullet_time(w);
            start_collision_replay(w);
        }
    }
}

// Helper: set all light uniforms for the main shader
static void set_lights(GLuint shader,
                       const Car& car,
                       const SpotLight* spots, int nSpots,
                       const StreetLight* streetLights, int nStreet,
                       const Vec3& viewPos,
                       float sceneTime,
                       bool streetLightsEnabled,
                       float stormBlend,
                       float lightningFlash,
                       float streetLightFlicker)
{
    float skyPulse = 0.5f + 0.5f * sinf(sceneTime * 0.05f);
    float stormDarken = 1.f - 0.56f * stormBlend;
    float lightningBoost = 1.f + 2.4f * lightningFlash;
    int carLightCount = car.headlightsOn ? 2 : 0;
    int streetCount = streetLightsEnabled ? nStreet : 0;
    int numLights = 1 + nSpots + streetCount + carLightCount;
    if (numLights > MAX_LIGHTS) numLights = MAX_LIGHTS;
    setInt(shader, "uNumLights", numLights);

    int idx = 0;

    // Sun
    setVec3(shader, "uLightPos[0]",   SUN_POS.x, SUN_POS.y, SUN_POS.z);
        setVec3(shader, "uLightColor[0]",
            (0.20f + 0.06f * skyPulse) * stormDarken * lightningBoost,
            (0.24f + 0.08f * skyPulse) * stormDarken * lightningBoost,
            (0.34f + 0.12f * skyPulse) * stormDarken * lightningBoost);
    idx = 1;

    char nameBuf[64];
    int spotStart = idx;
    int spotAdded = 0;
    for (int i = 0; i < nSpots && idx < MAX_LIGHTS; i++, idx++, spotAdded++) {
        Vec3 lp = spotlight_world_pos(spots[i]);
        Vec3 dir = spots[i].direction();
        snprintf(nameBuf, sizeof(nameBuf), "uLightPos[%d]", idx);
        setVec3(shader, nameBuf, lp.x, lp.y, lp.z);
        snprintf(nameBuf, sizeof(nameBuf), "uLightColor[%d]", idx);
        setVec3(shader, nameBuf,
            spots[i].colorR * (5.10f + 0.70f * stormBlend),
            spots[i].colorG * (5.10f + 0.70f * stormBlend),
            spots[i].colorB * (5.10f + 0.70f * stormBlend));
        snprintf(nameBuf, sizeof(nameBuf), "uSpotLightDir[%d]", spotAdded);
        setVec3(shader, nameBuf, dir.x, dir.y, dir.z);
    }
    setInt(shader, "uSpotStart", spotStart);
    setInt(shader, "uSpotCount", spotAdded);

    int streetStart = idx;
    int streetAdded = 0;
    for (int i = 0; i < streetCount && idx < MAX_LIGHTS; i++, idx++, streetAdded++) {
        const StreetLight& s = streetLights[i];
        Vec3 lp = street_lamp_pos(s);
        snprintf(nameBuf, sizeof(nameBuf), "uLightPos[%d]", idx);
        setVec3(shader, nameBuf, lp.x, lp.y, lp.z);
        snprintf(nameBuf, sizeof(nameBuf), "uLightColor[%d]", idx);
        float fl = std::max(0.04f, streetLightFlicker);
        setVec3(shader, nameBuf,
            s.colorR * fl * (2.35f + 0.42f * stormBlend),
            s.colorG * fl * (2.35f + 0.42f * stormBlend),
            s.colorB * fl * (2.35f + 0.42f * stormBlend));
    }

    setInt(shader, "uStreetStart", streetStart);
    setInt(shader, "uStreetCount", streetAdded);

    int carStart = idx;
    int carAdded = 0;
    Vec3 fwd = car_forward(car);
    setVec3(shader, "uCarLightDir[0]", fwd.x, fwd.y, fwd.z);
    setVec3(shader, "uCarLightDir[1]", fwd.x, fwd.y, fwd.z);
    if (car.headlightsOn) {
        const float offsets[2] = {-0.5f, 0.5f};
        for (int i = 0; i < 2 && idx < MAX_LIGHTS; i++, idx++, carAdded++) {
            CarLight light = headlight_state(car, offsets[i]);
            snprintf(nameBuf, sizeof(nameBuf), "uLightPos[%d]", idx);
            setVec3(shader, nameBuf, light.position.x, light.position.y, light.position.z);
            snprintf(nameBuf, sizeof(nameBuf), "uLightColor[%d]", idx);
            setVec3(shader, nameBuf, light.color.x, light.color.y, light.color.z);
        }
    }
    setInt(shader, "uCarStart", carStart);
    setInt(shader, "uCarCount", carAdded);

    setVec3(shader, "uViewPos", viewPos.x, viewPos.y, viewPos.z);
    float ambient = (car.headlightsOn ? 0.078f : 0.066f) + skyPulse * 0.014f;
    ambient *= (1.f - 0.55f * stormBlend);
    ambient += lightningFlash * 0.30f;
    setFloat(shader, "uAmbient", ambient);
}

static void draw_street_lights_lit(const World& w) {
    for (int i = 0; i < w.numStreetLights; i++) {
        const StreetLight& s = w.streetLights[i];
        const Vec3 dir = street_lamp_dir_xz(s);
        const Vec3 lp = street_lamp_pos(s);
        const float armYaw = atan2f(-dir.z, dir.x);
        const Vec3 top = {s.x, s.height - 0.05f, s.z};

        // Pole: scale cylinder in Y by height.
        float poleT[16], poleS[16], poleTmp[16], poleM[16];
        mat_translate(poleT, s.x, s.height * 0.5f, s.z);
        mat_scale(poleS, 1.f, s.height, 1.f);
        mat_mul(poleTmp, poleT, poleS);
        mat_identity(poleM);
        mat_mul(poleM, poleTmp, poleM);
        setMat4(w.mainShader, "uModel", poleM);
        setVec4(w.mainShader, "uColor", 0.26f, 0.28f, 0.31f, 1.f);
        setFloat(w.mainShader, "uShininess", 18.f);
        setInt(w.mainShader, "uUseTexture", 0);
        mesh_draw(w.streetPoleMesh);

        // Top collar
        float collarT[16], collarS[16], collarTmp[16], collarM[16];
        mat_translate(collarT, s.x, s.height - 0.18f, s.z);
        mat_scale(collarS, 0.34f, 0.22f, 0.34f);
        mat_mul(collarTmp, collarT, collarS);
        mat_identity(collarM);
        mat_mul(collarM, collarTmp, collarM);
        setMat4(w.mainShader, "uModel", collarM);
        setVec4(w.mainShader, "uColor", 0.33f, 0.35f, 0.38f, 1.f);
        setFloat(w.mainShader, "uShininess", 18.f);
        setInt(w.mainShader, "uUseTexture", 0);
        mesh_draw(w.spotMeshes.base);

        // Single horizontal arm from pole toward lamp.
        float armT[16], armS[16], armRY[16], armTmpA[16], armM[16];
        mat_translate(armT, top.x + dir.x * 0.55f, top.y, top.z + dir.z * 0.55f);
        mat_rotY(armRY, armYaw);
        mat_scale(armS, 1.10f, 0.12f, 0.12f);
        mat_mul(armTmpA, armRY, armS);
        mat_mul(armM, armT, armTmpA);
        setMat4(w.mainShader, "uModel", armM);
        setVec4(w.mainShader, "uColor", 0.38f, 0.40f, 0.44f, 1.f);
        setFloat(w.mainShader, "uShininess", 24.f);
        setInt(w.mainShader, "uUseTexture", 0);
        mesh_draw(w.spotMeshes.arm);

        // Vertical drop connector at lamp end.
        const float dropTopY = top.y;
        const float dropBottomY = lp.y + 0.30f;
        const float dropLen = std::max(0.10f, dropTopY - dropBottomY);
        const float dropCY = (dropTopY + dropBottomY) * 0.5f;

        float dropT[16], dropS[16], dropTmp[16], dropM[16];
        mat_translate(dropT, lp.x, dropCY, lp.z);
        mat_scale(dropS, 0.11f, dropLen, 0.11f);
        mat_mul(dropTmp, dropT, dropS);
        mat_identity(dropM);
        mat_mul(dropM, dropTmp, dropM);
        setMat4(w.mainShader, "uModel", dropM);
        setVec4(w.mainShader, "uColor", 0.30f, 0.32f, 0.35f, 1.f);
        setFloat(w.mainShader, "uShininess", 22.f);
        setInt(w.mainShader, "uUseTexture", 0);
        mesh_draw(w.streetPoleMesh);

        // Lamp housing ring + shell.
        float ringT[16], ringM[16];
        mat_translate(ringT, lp.x, lp.y + 0.12f, lp.z);
        mat_identity(ringM);
        mat_mul(ringM, ringT, ringM);
        setMat4(w.mainShader, "uModel", ringM);
        setVec4(w.mainShader, "uColor", 0.45f, 0.46f, 0.49f, 1.f);
        setFloat(w.mainShader, "uShininess", 30.f);
        setInt(w.mainShader, "uUseTexture", 0);
        mesh_draw(w.spotMeshes.ring);

        float lampT[16], lampM[16], lampS[16], lampTmp[16];
        mat_translate(lampT, lp.x, lp.y, lp.z);
    mat_scale(lampS, 0.88f, 0.64f, 0.64f);
        mat_mul(lampTmp, lampT, lampS);
        mat_identity(lampM);
        mat_mul(lampM, lampTmp, lampM);
        setMat4(w.mainShader, "uModel", lampM);
        if (w.streetLightsEnabled) setVec4(w.mainShader, "uColor", 0.20f, 0.21f, 0.24f, 1.f);
        else                       setVec4(w.mainShader, "uColor", 0.10f, 0.10f, 0.11f, 1.f);
        setFloat(w.mainShader, "uShininess", 32.f);
        setInt(w.mainShader, "uUseTexture", 0);
        mesh_draw(w.streetLampMesh);
    }
}

static void draw_street_lights_emissive(const World& w) {
    if (!w.streetLightsEnabled) return;
    float flicker = (w.stormBlend > 0.01f) ? std::max(0.04f, w.streetLightFlicker) : 1.f;
    for (int i = 0; i < w.numStreetLights; i++) {
        const StreetLight& s = w.streetLights[i];
        Vec3 lp = street_lamp_pos(s);
        float model[16], T[16], S[16], tmp[16];
        mat_translate(T, lp.x, lp.y, lp.z);
        mat_scale(S, 0.86f, 0.58f, 0.58f);
        mat_mul(tmp, T, S);
        mat_identity(model);
        mat_mul(model, tmp, model);

        setMat4(w.emissiveShader, "uModel", model);
        setVec4(w.emissiveShader, "uColor",
            s.colorR * flicker * 1.95f,
            s.colorG * flicker * 1.95f,
            s.colorB * flicker * 1.95f,
            1.0f);
        mesh_draw(w.streetLampMesh);

        // Soft glow halo so "ON" state is obvious from distance.
        float haloModel[16], haloT[16], haloS[16], haloTmp[16];
        mat_translate(haloT, lp.x, lp.y, lp.z);
        mat_scale(haloS, 1.45f, 1.05f, 1.05f);
        mat_mul(haloTmp, haloT, haloS);
        mat_identity(haloModel);
        mat_mul(haloModel, haloTmp, haloModel);
        setMat4(w.emissiveShader, "uModel", haloModel);
        setVec4(w.emissiveShader, "uColor",
            s.colorR * flicker * 0.70f,
            s.colorG * flicker * 0.70f,
            s.colorB * flicker * 0.70f,
            0.62f);
        mesh_draw(w.streetLampMesh);
    }
}

static void draw_trees(const World& w) {
    for (int i = 0; i < w.numTrees; i++) {
        const Tree& t = w.trees[i];

        float trunkT[16], trunkS[16], trunkRS[16], trunkM[16];
        mat_translate(trunkT, t.x, t.trunkHeight * 0.5f, t.z);
        mat_scale(trunkS, 1.22f, t.trunkHeight, 1.22f);
        mat_mul(trunkRS, trunkT, trunkS);
        mat_identity(trunkM);
        mat_mul(trunkM, trunkRS, trunkM);
        setMat4(w.mainShader, "uModel", trunkM);
        set_textured_style(w.mainShader, w.barkTex, 0.78f, 0.64f, 0.48f, 12.f);
        mesh_draw(w.treeTrunkMesh);

        float rootT[16], rootS[16], rootRS[16], rootM[16];
        mat_translate(rootT, t.x, t.trunkHeight * 0.11f, t.z);
        mat_scale(rootS, 1.48f, t.trunkHeight * 0.24f, 1.48f);
        mat_mul(rootRS, rootT, rootS);
        mat_identity(rootM);
        mat_mul(rootM, rootRS, rootM);
        setMat4(w.mainShader, "uModel", rootM);
        set_textured_style(w.mainShader, w.barkTex, 0.72f, 0.56f, 0.42f, 10.f);
        mesh_draw(w.treeTrunkMesh);

        float trunkCapT[16], trunkCapS[16], trunkCapRS[16], trunkCapM[16];
        mat_translate(trunkCapT, t.x, t.trunkHeight * 0.82f, t.z);
        mat_scale(trunkCapS, 0.92f, t.trunkHeight * 0.56f, 0.92f);
        mat_mul(trunkCapRS, trunkCapT, trunkCapS);
        mat_identity(trunkCapM);
        mat_mul(trunkCapM, trunkCapRS, trunkCapM);
        setMat4(w.mainShader, "uModel", trunkCapM);
        set_textured_style(w.mainShader, w.barkTex, 0.74f, 0.58f, 0.44f, 12.f);
        mesh_draw(w.treeTrunkMesh);

        float branchA_T[16], branchA_S[16], branchA_RS[16], branchA_M[16];
        mat_translate(branchA_T, t.x - 0.35f, t.trunkHeight * 0.78f, t.z + 0.10f);
        mat_scale(branchA_S, 0.42f, t.trunkHeight * 0.22f, 0.42f);
        mat_mul(branchA_RS, branchA_T, branchA_S);
        mat_identity(branchA_M);
        mat_mul(branchA_M, branchA_RS, branchA_M);
        setMat4(w.mainShader, "uModel", branchA_M);
        set_textured_style(w.mainShader, w.barkTex, 0.70f, 0.54f, 0.40f, 10.f);
        mesh_draw(w.treeTrunkMesh);

        float branchB_T[16], branchB_S[16], branchB_RS[16], branchB_M[16];
        mat_translate(branchB_T, t.x + 0.38f, t.trunkHeight * 0.74f, t.z - 0.10f);
        mat_scale(branchB_S, 0.38f, t.trunkHeight * 0.20f, 0.38f);
        mat_mul(branchB_RS, branchB_T, branchB_S);
        mat_identity(branchB_M);
        mat_mul(branchB_M, branchB_RS, branchB_M);
        setMat4(w.mainShader, "uModel", branchB_M);
        set_textured_style(w.mainShader, w.barkTex, 0.68f, 0.52f, 0.38f, 10.f);
        mesh_draw(w.treeTrunkMesh);

        float canopyT[16], canopyS[16], canopyRS[16], canopyM[16];
        mat_translate(canopyT, t.x, t.trunkHeight + t.canopyRadius * 0.62f, t.z);
        mat_scale(canopyS, t.canopyRadius * 1.36f, t.canopyRadius * 0.96f, t.canopyRadius * 1.30f);
        mat_mul(canopyRS, canopyT, canopyS);
        mat_identity(canopyM);
        mat_mul(canopyM, canopyRS, canopyM);
        setMat4(w.mainShader, "uModel", canopyM);
        set_textured_style(w.mainShader, w.leafTex, 0.64f, 0.86f, 0.56f, 14.f);
        mesh_draw(w.treeCanopyMesh);

        float canopy2T[16], canopy2S[16], canopy2RS[16], canopy2M[16];
        mat_translate(canopy2T, t.x - t.canopyRadius * 0.42f, t.trunkHeight + t.canopyRadius * 0.28f, t.z + t.canopyRadius * 0.30f);
        mat_scale(canopy2S, t.canopyRadius * 0.90f, t.canopyRadius * 0.70f, t.canopyRadius * 0.88f);
        mat_mul(canopy2RS, canopy2T, canopy2S);
        mat_identity(canopy2M);
        mat_mul(canopy2M, canopy2RS, canopy2M);
        setMat4(w.mainShader, "uModel", canopy2M);
        set_textured_style(w.mainShader, w.leafTex, 0.56f, 0.82f, 0.48f, 14.f);
        mesh_draw(w.treeCanopyMesh);

        float canopy3T[16], canopy3S[16], canopy3RS[16], canopy3M[16];
        mat_translate(canopy3T, t.x + t.canopyRadius * 0.44f, t.trunkHeight + t.canopyRadius * 0.26f, t.z - t.canopyRadius * 0.28f);
        mat_scale(canopy3S, t.canopyRadius * 0.86f, t.canopyRadius * 0.66f, t.canopyRadius * 0.82f);
        mat_mul(canopy3RS, canopy3T, canopy3S);
        mat_identity(canopy3M);
        mat_mul(canopy3M, canopy3RS, canopy3M);
        setMat4(w.mainShader, "uModel", canopy3M);
        set_textured_style(w.mainShader, w.leafTex, 0.58f, 0.84f, 0.50f, 14.f);
        mesh_draw(w.treeCanopyMesh);

        float canopyTopT[16], canopyTopS[16], canopyTopRS[16], canopyTopM[16];
        mat_translate(canopyTopT, t.x, t.trunkHeight + t.canopyRadius * 1.16f, t.z);
        mat_scale(canopyTopS, t.canopyRadius * 0.74f, t.canopyRadius * 0.78f, t.canopyRadius * 0.72f);
        mat_mul(canopyTopRS, canopyTopT, canopyTopS);
        mat_identity(canopyTopM);
        mat_mul(canopyTopM, canopyTopRS, canopyTopM);
        setMat4(w.mainShader, "uModel", canopyTopM);
        set_textured_style(w.mainShader, w.leafTex, 0.70f, 0.90f, 0.60f, 16.f);
        mesh_draw(w.treeCanopyMesh);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

static void draw_storm_ground_wetness(const World& w) {
    if (w.stormBlend <= 0.01f) return;

    const float wet = clamp01(w.stormBlend);
    glDepthMask(GL_FALSE);

    float wetGroundModel[16], wetRoadModel[16];
    mat_translate(wetGroundModel, 0.f, TRACK_GROUND_Y + 0.070f, 0.f);
    mat_translate(wetRoadModel,   0.f, TRACK_ROAD_Y + 0.070f, 0.f);

    setMat4(w.mainShader, "uModel", wetGroundModel);
    setVec4(w.mainShader, "uColor",
            0.10f + 0.05f * wet,
            0.17f + 0.08f * wet,
            0.20f + 0.10f * wet,
            0.14f + 0.22f * wet);
        setFloat(w.mainShader, "uShininess", 52.f);
    setInt(w.mainShader, "uUseTexture", 0);
    mesh_draw(w.track.groundMesh);

    setMat4(w.mainShader, "uModel", wetRoadModel);
    setVec4(w.mainShader, "uColor",
            0.16f + 0.05f * wet,
            0.18f + 0.05f * wet,
            0.23f + 0.08f * wet,
            0.16f + 0.24f * wet);
        setFloat(w.mainShader, "uShininess", 72.f);
    setInt(w.mainShader, "uUseTexture", 0);
    mesh_draw(w.track.roadMesh);

    glDepthMask(GL_TRUE);
}

static void draw_storm_rain_emissive(const World& w, const Vec3& eyePos) {
    if (w.stormBlend <= 0.01f) return;

    const float wet = clamp01(w.stormBlend);
    const int dropCount = STORM_RAIN_DROPS_MIN +
                          (int)((float)(STORM_RAIN_DROPS_MAX - STORM_RAIN_DROPS_MIN) * wet);

    glDepthMask(GL_FALSE);
    for (int i = 0; i < dropCount; i++) {
        float hx = hash01(i * 37 + 11);
        float hz = hash01(i * 53 + 17);
        float hs = hash01(i * 29 + 5);
        float hp = hash01(i * 97 + 19);

        float x = eyePos.x + (hx * 2.f - 1.f) * STORM_RAIN_RADIUS_X;
        float z = eyePos.z + (hz * 2.f - 1.f) * STORM_RAIN_RADIUS_Z;
        float speed = 18.f + hs * 26.f;
        float y = STORM_RAIN_TOP_Y - fmodf(w.stormRainScroll * speed + hp * STORM_RAIN_VERTICAL_SPAN,
                                           STORM_RAIN_VERTICAL_SPAN);

        float sway = sinf(w.sceneTime * (1.2f + hs * 1.8f) + hp * 6.283f) * (0.20f + 0.55f * wet);
        x += sway;

        float T[16], S[16], M[16];
        mat_translate(T, x, y, z);
        mat_scale(S, 0.035f, 0.80f + wet * 0.65f, 0.035f);
        mat_mul(M, T, S);
        setMat4(w.emissiveShader, "uModel", M);
        setVec4(w.emissiveShader, "uColor",
                0.58f + 0.20f * wet,
                0.66f + 0.18f * wet,
                0.78f + 0.16f * wet,
                0.16f + 0.22f * wet);
        mesh_draw(w.streetPoleMesh);
    }
    glDepthMask(GL_TRUE);
}

static void draw_lightning_emissive(const World& w) {
    if (w.lightningFlash <= 0.01f || w.stormBlend <= 0.01f) return;

    const float flash = clamp01(w.lightningFlash);
    const float pulse = 0.74f + 0.26f * fabsf(sinf(w.sceneTime * 88.f));
    const float jitter = sinf(w.sceneTime * 42.f) * 0.32f;
    const float boltX = w.lightningStrikeX;
    const float boltZ = w.lightningStrikeZ;

    glDepthMask(GL_FALSE);

    for (int seg = 0; seg < 6; seg++) {
        float segH = 3.3f;
        float topY = 19.8f - (float)seg * segH;
        float centerY = topY - segH * 0.5f;
        float sx = boltX + ((seg % 2) ? 0.42f : -0.30f) + jitter * ((float)seg + 1.f) * 0.8f;
        float sz = boltZ + ((seg % 2) ? -0.30f : 0.26f) + sinf(w.sceneTime * 20.f + seg) * 0.07f;

        float T[16], R[16], S[16], RS[16], M[16];
        mat_translate(T, sx, centerY, sz);
        mat_rotZ(R, ((seg % 2) ? -0.28f : 0.22f) + jitter * 0.28f);
        mat_scale(S, 0.19f, segH, 0.19f);
        mat_mul(RS, R, S);
        mat_mul(M, T, RS);

        setMat4(w.emissiveShader, "uModel", M);
        setVec4(w.emissiveShader, "uColor",
            (0.88f + 0.12f * flash) * pulse,
            (0.93f + 0.07f * flash) * pulse,
            1.00f,
            (0.60f + 0.40f * flash) * pulse);
        mesh_draw(w.streetPoleMesh);
    }

    // Side branches for a less uniform, more natural strike shape.
    for (int b = 0; b < 3; b++) {
        float by = 13.8f - (float)b * 3.6f;
        float bx = boltX + ((b % 2) ? 0.75f : -0.72f);
        float bz = boltZ + ((b % 2) ? -0.55f : 0.48f);
        float T[16], R[16], S[16], RS[16], M[16];
        mat_translate(T, bx, by, bz);
        mat_rotZ(R, (b % 2) ? -0.95f : 0.92f);
        mat_scale(S, 0.12f, 1.95f, 0.12f);
        mat_mul(RS, R, S);
        mat_mul(M, T, RS);
        setMat4(w.emissiveShader, "uModel", M);
        setVec4(w.emissiveShader, "uColor",
                0.80f * pulse,
                0.88f * pulse,
                1.00f,
                (0.34f + 0.24f * flash) * pulse);
        mesh_draw(w.streetPoleMesh);
    }

    float haloT[16], haloS[16], haloM[16];
    mat_translate(haloT, boltX, 0.35f, boltZ);
        mat_scale(haloS, 4.2f + 3.2f * flash, 0.18f, 4.2f + 3.2f * flash);
    mat_mul(haloM, haloT, haloS);
    setMat4(w.emissiveShader, "uModel", haloM);
        setVec4(w.emissiveShader, "uColor",
                0.84f * pulse,
                0.88f * pulse,
                1.0f,
                (0.36f + 0.38f * flash) * pulse);
    mesh_draw(w.streetLampMesh);

    glDepthMask(GL_TRUE);
}

void world_render(World& w, int screenW, int screenH) {
    // Compute matrices
    float view[16], proj[16];
    camera_compute_view(w.camera, w.car, w.spotlights, view);
    mat_perspective(proj, 60.f, (float)screenW / (float)screenH, 0.1f, 500.f);

    // Recover camera world position from the view matrix.
    float eyeW[3];
    eyeW[0] = -(view[0] * view[12] + view[1] * view[13] + view[2]  * view[14]);
    eyeW[1] = -(view[4] * view[12] + view[5] * view[13] + view[6]  * view[14]);
    eyeW[2] = -(view[8] * view[12] + view[9] * view[13] + view[10] * view[14]);
    Vec3 eyePos = {eyeW[0], eyeW[1], eyeW[2]};

    // ---- Main shader pass ----
    glUseProgram(w.mainShader);
    setMat4(w.mainShader, "uView",       view);
    setMat4(w.mainShader, "uProjection", proj);
    set_lights(w.mainShader,
               w.car,
               w.spotlights, NUM_BUILDINGS,
               w.streetLights, w.numStreetLights,
               eyePos, w.sceneTime, w.streetLightsEnabled,
               w.stormBlend, w.lightningFlash, w.streetLightFlicker);

    track_draw(w.track, w.mainShader);
    draw_storm_ground_wetness(w);
    draw_trees(w);
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        building_draw(w.buildings[i], w.buildingMeshes, w.mainShader);
        fan_draw(w.fans[i], w.buildings[i], w.fanMeshes, w.mainShader);
        spotlight_draw_gimbal(w.spotlights[i], w.spotMeshes, w.mainShader);
    }
    draw_street_lights_lit(w);
    wall_draw(w.wall, w.mainShader);
    car_draw(w.car, w.mainShader);

    // ---- Emissive shader pass (spotlight markers + street lamps) ----
    glUseProgram(w.emissiveShader);
    setMat4(w.emissiveShader, "uView",       view);
    setMat4(w.emissiveShader, "uProjection", proj);
    for (int i = 0; i < NUM_BUILDINGS; i++)
        spotlight_draw_marker(w.spotlights[i], w.spotMeshes, w.emissiveShader);
    draw_street_lights_emissive(w);
    draw_storm_rain_emissive(w, eyePos);
    draw_lightning_emissive(w);
}

void world_reset(World& w) {
    car_reset(w.car);
    w.bulletTimeRemaining = 0.f;
    w.replayActive = false;
    w.replayElapsed = 0.f;
    w.replayFrameCount = 0;
    w.postCrashHold = 0.f;
    w.historyHead = 0;
    w.historyCount = 0;
    w.streetLightsEnabled = true;
    w.sceneTime = 0.f;

    w.stormRainScroll = 0.f;
    w.lightningCooldown = 0.8f;
    w.lightningFlash = 0.f;
    w.lightningStrikeX = 0.f;
    w.lightningStrikeZ = 0.f;
    w.lightningSeed = 0;
    w.streetLightFlicker = 1.f;
    w.carVelX = 0.f;
    w.carVelZ = 0.f;
    w.stormSkidPhase = 0.f;
}

void world_cleanup(World& w) {
    if (w.mainShader)     glDeleteProgram(w.mainShader);
    if (w.emissiveShader) glDeleteProgram(w.emissiveShader);
    if (w.brickTex)       glDeleteTextures(1, &w.brickTex);
    if (w.woodTex)        glDeleteTextures(1, &w.woodTex);
    if (w.stoneTex)       glDeleteTextures(1, &w.stoneTex);
    if (w.plasterTex)     glDeleteTextures(1, &w.plasterTex);
    if (w.roadTex)        glDeleteTextures(1, &w.roadTex);
    if (w.grassTex)       glDeleteTextures(1, &w.grassTex);
    if (w.barkTex)        glDeleteTextures(1, &w.barkTex);
    if (w.leafTex)        glDeleteTextures(1, &w.leafTex);
    if (w.carTex)        glDeleteTextures(1, &w.carTex);
    if (w.carMetalTex)   glDeleteTextures(1, &w.carMetalTex);
    if (w.carRubberTex)  glDeleteTextures(1, &w.carRubberTex);

    mesh_free(w.car.bodyMesh);
    mesh_free(w.car.cabinMesh);
    mesh_free(w.car.wheelMesh);
    mesh_free(w.car.headlightMesh);
    mesh_free(w.car.headlightBeamMesh);
    mesh_free(w.track.roadMesh);
    mesh_free(w.track.groundMesh);
    mesh_free(w.track.laneStripeMesh);
    mesh_free(w.track.curbInnerMesh);
    mesh_free(w.track.curbOuterMesh);
    mesh_free(w.buildingMeshes.slab);
    mesh_free(w.fanMeshes.tower);
    mesh_free(w.fanMeshes.beam);
    mesh_free(w.fanMeshes.hub);
    mesh_free(w.fanMeshes.sail);
    mesh_free(w.fanMeshes.cap);
    mesh_free(w.spotMeshes.sphere);
    mesh_free(w.spotMeshes.ring);
    mesh_free(w.spotMeshes.arm);
    mesh_free(w.spotMeshes.base);
    mesh_free(w.spotMeshes.housing);
    mesh_free(w.streetPoleMesh);
    mesh_free(w.streetLampMesh);
    mesh_free(w.treeTrunkMesh);
    mesh_free(w.treeCanopyMesh);
    for (int i = 0; i < 4; i++)
        mesh_free(w.wall.panels[i]);
}

void world_clear_color(const World& w, float& r, float& g, float& b) {
    float skyPulse = 0.5f + 0.5f * sinf(w.sceneTime * 0.08f);
    float clearR = 0.004f + skyPulse * 0.010f;
    float clearG = 0.006f + skyPulse * 0.012f;
    float clearB = 0.022f + skyPulse * 0.028f;

    float stormR = 0.008f;
    float stormG = 0.010f;
    float stormB = 0.022f;

    float t = clamp01(w.stormBlend);
    r = clearR * (1.f - t) + stormR * t;
    g = clearG * (1.f - t) + stormG * t;
    b = clearB * (1.f - t) + stormB * t;

    float flash = clamp01(w.lightningFlash);
    r += flash * 0.34f;
    g += flash * 0.40f;
    b += flash * 0.52f;
}

void world_toggle_street_lights(World& w) {
    w.streetLightsEnabled = !w.streetLightsEnabled;
}

void world_toggle_storm(World& w) {
    w.stormEnabled = !w.stormEnabled;
}

bool world_storm_active(const World& w) {
    return w.stormEnabled;
}
