#include "shader.h"
#include <fstream>
#include <sstream>
#include <iostream>

static std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) {
        std::cerr << "[shader] Cannot open: " << path << "\n";
        return "";
    }
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static GLuint compileShader(GLenum type, const std::string& src, const std::string& name) {
    const char* c = src.c_str();
    GLuint id = glCreateShader(type);
    glShaderSource(id, 1, &c, nullptr);
    glCompileShader(id);
    GLint ok; glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetShaderInfoLog(id, 512, nullptr, log);
        std::cerr << "[shader] Compile error (" << name << "):\n" << log << "\n";
        glDeleteShader(id); return 0;
    }
    return id;
}

GLuint createShaderProgram(const std::string& vertPath, const std::string& fragPath) {
    std::string vs = readFile(vertPath), fs = readFile(fragPath);
    if (vs.empty() || fs.empty()) return 0;
    GLuint vert = compileShader(GL_VERTEX_SHADER,   vs, vertPath);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fs, fragPath);
    if (!vert || !frag) { glDeleteShader(vert); glDeleteShader(frag); return 0; }
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vert); glAttachShader(prog, frag);
    glLinkProgram(prog);
    glDeleteShader(vert); glDeleteShader(frag);
    GLint ok; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512]; glGetProgramInfoLog(prog, 512, nullptr, log);
        std::cerr << "[shader] Link error:\n" << log << "\n";
        glDeleteProgram(prog); return 0;
    }
    return prog;
}

void setInt  (GLuint p, const char* n, int v)   { glUniform1i (glGetUniformLocation(p,n), v); }
void setFloat(GLuint p, const char* n, float v) { glUniform1f (glGetUniformLocation(p,n), v); }
void setVec3 (GLuint p, const char* n, float x, float y, float z)              { glUniform3f (glGetUniformLocation(p,n), x, y, z); }
void setVec4 (GLuint p, const char* n, float x, float y, float z, float w)     { glUniform4f (glGetUniformLocation(p,n), x, y, z, w); }
void setMat4 (GLuint p, const char* n, const float* m) { glUniformMatrix4fv(glGetUniformLocation(p,n), 1, GL_FALSE, m); }
