#pragma once
#include <glad/glad.h>

// Generate simple procedural textures (no external image files needed).
// Returns a GL texture object ready to bind to GL_TEXTURE_2D.
GLuint gen_brick_texture();
GLuint gen_wood_texture();
GLuint gen_stone_texture();
GLuint gen_plaster_texture();
GLuint gen_road_texture();  // dark asphalt with faint lane markings
GLuint gen_grass_texture();
GLuint gen_bark_texture();
GLuint gen_leaf_texture();
GLuint gen_car_paint_texture();
GLuint gen_car_metal_texture();
GLuint gen_car_rubber_texture();
