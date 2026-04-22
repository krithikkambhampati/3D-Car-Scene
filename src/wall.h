#pragma once
#include <glad/glad.h>
#include "mesh.h"
#include "math_utils.h"
#include "constants.h"
// ============================================================
//  wall.h  –  Rectangular boundary wall around the arena.
// ============================================================
struct Wall {
    Mesh panels[4]; // N, S, E, W
};

void wall_init(Wall& w);
void wall_draw(const Wall& w, GLuint shader);
