#pragma once
// ============================================================
//  constants.h  –  scene-wide compile-time constants.
//  Change any value here to retune the scene without touching
//  logic code.
// ============================================================

static constexpr float PI_F = 3.14159265f;

// ---------- Arena / Wall ------------------------------------
constexpr float ARENA_HALF_W  = 55.0f;
constexpr float ARENA_HALF_D  = 35.0f;
constexpr float WALL_HEIGHT   = 10.0f;
constexpr float WALL_THICK    =  1.0f;

// ---------- Track -------------------------------------------
constexpr float TRACK_A       = 30.0f;  // semi-major (X)
constexpr float TRACK_B       = 18.0f;  // semi-minor (Z)
constexpr float TRACK_WIDTH   = 10.0f;  // road width
constexpr int   TRACK_SEGS    = 80;     // segments around oval
constexpr float TRACK_GROUND_Y = -0.18f;
constexpr float TRACK_ROAD_Y   =  0.018f;
constexpr float TRACK_STRIPE_Y =  0.035f;
constexpr float TRACK_CURB_INNER_Y = 0.050f;
constexpr float TRACK_CURB_OUTER_Y = 0.062f;

// ---------- Car ---------------------------------------------
constexpr float CAR_BODY_HW   = 1.0f;   // half-width  (X)
constexpr float CAR_BODY_HH   = 0.55f;  // half-height (Y)
constexpr float CAR_BODY_HL   = 1.8f;   // half-length (Z)
constexpr float CAR_BODY_Y    = 0.6f;   // body center height above ground
constexpr float CAR_CABIN_HW  = 0.75f;
constexpr float CAR_CABIN_HH  = 0.42f;
constexpr float CAR_CABIN_HL  = 0.95f;
constexpr float CAR_WHEEL_R   = 0.38f;  // wheel radius
constexpr float CAR_WHEEL_W   = 0.32f;  // wheel half-width along axle
constexpr float CAR_RADIUS    = 2.5f;   // legacy: bounding circle for arena check
constexpr float CAR_HALF_W    = 1.2f;   // half-width for OBB collision
constexpr float CAR_HALF_H    = 2.1f;   // half-height (length) for OBB collision
constexpr float CAR_SPEED_INCR = 0.5f;
constexpr float CAR_MAX_SPEED = 20.0f;
constexpr float CAR_MIN_SPEED = -10.0f;
constexpr float CAR_TURN_DEG  =  3.0f;  // degrees per key press

// ---------- Buildings ---------------------------------------
constexpr int   NUM_BUILDINGS = 5;

// ---------- Fans --------------------------------------------
constexpr float FAN_DEFAULT_SPEED = 1.5f; // rad/s
constexpr float FAN_SPEED_INCR    = 0.3f;
constexpr float FAN_MAX_SPEED     = 8.0f;
constexpr float FAN_MIN_SPEED     = 0.0f;
constexpr float FAN_BLADE_L       = 2.0f; // blade half-length
constexpr float FAN_BLADE_W       = 0.16f;
constexpr float FAN_BLADE_TH      = 0.06f;
constexpr int   FAN_BLADES        = 4;

// ---------- Spotlights --------------------------------------
constexpr float LIGHT_SWING_SPEED = 0.7f;   // rad/s
constexpr float LIGHT_SWING_MAX   = 0.5236f; // 30° in radians

// ---------- Rendering ---------------------------------------
constexpr int   MAX_LIGHTS        = 12;

// ---------- Thunderstorm -----------------------------------
constexpr int   STORM_RAIN_DROPS_MIN      = 420;
constexpr int   STORM_RAIN_DROPS_MAX      = 900;
constexpr float STORM_RAIN_RADIUS_X       = 40.0f;
constexpr float STORM_RAIN_RADIUS_Z       = 28.0f;
constexpr float STORM_RAIN_TOP_Y          = 20.0f;
constexpr float STORM_RAIN_VERTICAL_SPAN  = 24.0f;
constexpr float STORM_LIGHTNING_MIN_GAP   = 0.9f;
constexpr float STORM_LIGHTNING_MAX_GAP   = 3.6f;
constexpr float STORM_SLIP_ALIGN_CLEAR    = 8.5f;
constexpr float STORM_SLIP_ALIGN_WET      = 2.1f;

// ---------- Bullet Time -------------------------------------
constexpr float BULLET_TIME_SCALE     = 0.18f;
constexpr float BULLET_TIME_DURATION  = 2.2f;
