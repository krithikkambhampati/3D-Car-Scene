#include "wall.h"
#include "shader.h"
#include "math_utils.h"

void wall_init(Wall& w) {
    float aw = ARENA_HALF_W, ad = ARENA_HALF_D, wh = WALL_HEIGHT, wt = WALL_THICK;
    // N panel (z = -ad): spans full width along X
    build_wall_quad(w.panels[0], aw * 2.f, wh, wt);
    // S panel (z = +ad)
    build_wall_quad(w.panels[1], aw * 2.f, wh, wt);
    // W panel (x = -aw): spans full depth along Z
    build_wall_quad(w.panels[2], wt, wh, ad * 2.f);
    // E panel (x = +aw)
    build_wall_quad(w.panels[3], wt, wh, ad * 2.f);
}

void wall_draw(const Wall& w, GLuint shader) {
    float aw = ARENA_HALF_W, ad = ARENA_HALF_D, wh = WALL_HEIGHT;
    float halfH = wh * 0.5f;

    struct { float tx, ty, tz; } positions[4] = {
        {  0.f,    halfH, -ad },  // N
        {  0.f,    halfH,  ad },  // S
        { -aw,     halfH,  0.f},  // W
        {  aw,     halfH,  0.f},  // E
    };

    for (int i = 0; i < 4; i++) {
        float model[16];
        mat_translate(model, positions[i].tx, positions[i].ty, positions[i].tz);
        setMat4 (shader, "uModel",      model);
        setVec4 (shader, "uColor",      0.10f, 0.12f, 0.16f, 1.f);
        setFloat(shader, "uShininess",  4.f);
        setInt  (shader, "uUseTexture", 0);
        mesh_draw(w.panels[i]);
    }
}
