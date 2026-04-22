#pragma once
#include <glad/glad.h>
#include "car.h"
#include "track.h"
#include "building.h"
#include "fan.h"
#include "light_source.h"
#include "wall.h"
#include "camera_system.h"
#include "constants.h"
// ============================================================
//  world.h  –  Owns all scene objects; orchestrates update & render.
// ============================================================

struct StreetLight {
    float x = 0.f, z = 0.f;
    float height = 8.f;
    float colorR = 1.f, colorG = 0.9f, colorB = 0.75f;
    float collisionRadius = 1.0f;
};

struct Tree {
    float x = 0.f, z = 0.f;
    float trunkHeight = 3.2f;
    float canopyRadius = 1.6f;
    float collisionRadius = 1.4f;
};

struct CarLight {
    Vec3 position;
    Vec3 color;
};

struct CarReplayState {
    float x = 0.f, z = 0.f, heading = 0.f, speed = 0.f;
};

struct World {
    // Shaders
    GLuint mainShader     = 0;
    GLuint emissiveShader = 0;

    // Scene objects
    Car          car;
    Track        track;
    Building     buildings[NUM_BUILDINGS];
    Fan          fans[NUM_BUILDINGS];
    SpotLight    spotlights[NUM_BUILDINGS];
    Wall         wall;
    CameraSystem camera;

    // Shared meshes
    BuildingMeshes buildingMeshes;
    FanMeshes      fanMeshes;
    SpotMeshes     spotMeshes;
    Mesh           streetPoleMesh;
    Mesh           streetLampMesh;
    Mesh           treeTrunkMesh;
    Mesh           treeCanopyMesh;

    // Textures
    GLuint brickTex = 0, woodTex = 0, stoneTex = 0, plasterTex = 0;
    GLuint roadTex = 0, grassTex = 0, barkTex = 0, leafTex = 0;
    GLuint carTex = 0;
    GLuint carMetalTex = 0;
    GLuint carRubberTex = 0;

    // Decorative/functional street lights
    StreetLight streetLights[4];
    int numStreetLights = 4;
    bool streetLightsEnabled = true;
    Tree trees[6];
    int numTrees = 6;
    float sceneTime = 0.f;

    // Collision bullet-time state
    float bulletTimeRemaining = 0.f;
    float postCrashHold = 0.f;

    // Replay-style bullet time (recent movement playback on collision)
    static constexpr int REPLAY_BUFFER_CAP = 320;
    CarReplayState history[REPLAY_BUFFER_CAP];
    int historyHead = 0;
    int historyCount = 0;

    CarReplayState replayFrames[REPLAY_BUFFER_CAP];
    int replayFrameCount = 0;
    bool replayActive = false;
    float replayElapsed = 0.f;
    float replayDuration = 3.4f;
    CarReplayState crashState;

    // Thunderstorm state
    bool stormEnabled = false;
    float stormBlend = 0.f;          // 0 = clear, 1 = full storm
    float stormRainScroll = 0.f;
    float lightningCooldown = 1.2f;
    float lightningFlash = 0.f;      // brief brightness spike
    float lightningStrikeX = 0.f;
    float lightningStrikeZ = 0.f;
    int lightningSeed = 0;
    float streetLightFlicker = 1.f;

    // Building window lighting
    bool buildingWindowsOn = false;   // are building windows lit?
    float windowFlickerPhase = 0.f;   // for flicker animation during storms
    float windowFlickerIntensity = 1.f; // intensity varies with storm

    // Storm driving feel (slip / drift)
    float carVelX = 0.f;
    float carVelZ = 0.f;
    float stormSkidPhase = 0.f;
};

void world_init   (World& w);
void world_update (World& w, float dt);
void world_render (World& w, int screenW, int screenH);
void world_reset  (World& w);
void world_cleanup(World& w);
void world_clear_color(const World& w, float& r, float& g, float& b);
void world_toggle_street_lights(World& w);
void world_toggle_storm(World& w);
void world_toggle_building_windows(World& w);
bool world_storm_active(const World& w);
