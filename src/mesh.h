#pragma once
#include <glad/glad.h>
#include <vector>
// ============================================================
//  mesh.h  –  GPU mesh upload and primitive builders.
//
//  Vertex layout (8 floats):
//    location 0: vec3 position
//    location 1: vec3 normal
//    location 2: vec2 texcoord
// ============================================================

struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLsizei count   = 0;
    bool    indexed = true;
};

// Upload vertex + index data to the GPU.
void mesh_upload(Mesh& mesh,
                 const std::vector<float>&        verts,
                 const std::vector<unsigned int>& indices);

// Free GPU resources.
void mesh_free(Mesh& mesh);

// Draw the mesh (assumes correct shader/VAO setup).
void mesh_draw(const Mesh& mesh);

// ---------- Primitive builders (allocate + upload) ----------
void build_box          (Mesh& m, float hw, float hh, float hd);
void build_cylinder     (Mesh& m, float radius, float halfH, int slices = 20);
void build_oval_track   (Mesh& m, float a, float b, float width, int segs = 80);
void build_ground_plane (Mesh& m, float halfW, float halfD, float uRepeat = 1.0f, float vRepeat = 1.0f);
void build_wall_quad    (Mesh& m, float width, float height, float depth = 1.0f);
void build_sphere       (Mesh& m, float radius, int stacks = 12, int slices = 20);
