#version 330 core
// ============================================================
//  vertex.glsl  –  Main scene vertex shader.
//  Input layout:
//    location 0: vec3 aPos      (position)
//    location 1: vec3 aNormal   (surface normal)
//    location 2: vec2 aTexCoord (uv)
// ============================================================

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vFragPos;     // world-space position
out vec3 vNormal;      // world-space normal
out vec2 vTexCoord;

void main() {
    vec4 worldPos   = uModel * vec4(aPos, 1.0);
    vFragPos        = vec3(worldPos);
    // Normal matrix: handles non-uniform scaling correctly
    vNormal         = mat3(transpose(inverse(uModel))) * aNormal;
    vTexCoord       = aTexCoord;
    gl_Position     = uProjection * uView * worldPos;
}
