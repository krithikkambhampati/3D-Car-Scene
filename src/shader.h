#pragma once
#include <glad/glad.h>
#include <string>

// Compile a vertex + fragment shader from files on disk.
// Returns 0 on failure (errors are printed to stderr).
GLuint createShaderProgram(const std::string& vertPath,
                           const std::string& fragPath);

// Convenience uniform setters (bind program before calling)
void setInt  (GLuint prog, const char* name, int v);
void setFloat(GLuint prog, const char* name, float v);
void setVec3 (GLuint prog, const char* name, float x, float y, float z);
void setVec4 (GLuint prog, const char* name, float x, float y, float z, float w);
void setMat4 (GLuint prog, const char* name, const float* m);
