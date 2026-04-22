#pragma once
#include <cmath>
#include "constants.h"
// ============================================================
//  math_utils.h  –  Column-major 4×4 matrix math + Vec3.
//  All float[16] arrays are in column-major (OpenGL) order.
//  Index mapping: m[col*4 + row]
// ============================================================

// ------------------------------------------------------------------
// Vec3 – simple 3-component vector
// ------------------------------------------------------------------
struct Vec3 {
    float x, y, z;
    Vec3(float x = 0.f, float y = 0.f, float z = 0.f) : x(x), y(y), z(z) {}

    Vec3  operator+(const Vec3& o) const { return {x+o.x, y+o.y, z+o.z}; }
    Vec3  operator-(const Vec3& o) const { return {x-o.x, y-o.y, z-o.z}; }
    Vec3  operator*(float s)       const { return {x*s,   y*s,   z*s  }; }
    Vec3  operator-()              const { return {-x,    -y,    -z   }; }
    Vec3& operator+=(const Vec3& o){ x+=o.x; y+=o.y; z+=o.z; return *this; }

    float dot  (const Vec3& o) const { return x*o.x + y*o.y + z*o.z; }
    Vec3  cross(const Vec3& o) const {
        return { y*o.z - z*o.y, z*o.x - x*o.z, x*o.y - y*o.x };
    }
    float length()    const { return sqrtf(x*x + y*y + z*z); }
    Vec3  normalized() const {
        float l = length();
        if (l < 1e-7f) return {0,0,0};
        return {x/l, y/l, z/l};
    }
};

// ------------------------------------------------------------------
// 4×4 column-major matrix helpers
// ------------------------------------------------------------------
inline void mat_identity(float* m) {
    for (int i = 0; i < 16; i++) m[i] = 0.f;
    m[0] = m[5] = m[10] = m[15] = 1.f;
}

inline void mat_mul(float* out, const float* a, const float* b) {
    // out = a * b  (both column-major)
    for (int col = 0; col < 4; col++)
        for (int row = 0; row < 4; row++) {
            float v = 0.f;
            for (int k = 0; k < 4; k++)
                v += a[k*4+row] * b[col*4+k];
            out[col*4+row] = v;
        }
}

inline void mat_translate(float* m, float tx, float ty, float tz) {
    mat_identity(m);
    m[12] = tx; m[13] = ty; m[14] = tz;
}

inline void mat_scale(float* m, float sx, float sy, float sz) {
    mat_identity(m);
    m[0] = sx; m[5] = sy; m[10] = sz;
}

// Rotation around Y axis (column-major)
inline void mat_rotY(float* m, float angle) {
    mat_identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;  m[2] = -s;   // column 0
    m[8] = s;  m[10] =  c;  // column 2
}

// Rotation around X axis (column-major)
inline void mat_rotX(float* m, float angle) {
    mat_identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[5] = c;  m[6] =  s;   // column 1
    m[9] = -s; m[10] =  c;  // column 2
}

// Rotation around Z axis (column-major)
inline void mat_rotZ(float* m, float angle) {
    mat_identity(m);
    float c = cosf(angle), s = sinf(angle);
    m[0] = c;  m[1] =  s;   // column 0
    m[4] = -s; m[5] =  c;   // column 1
}

// LookAt view matrix (column-major)
inline void mat_lookAt(float* m, Vec3 eye, Vec3 center, Vec3 up) {
    Vec3 f = (center - eye).normalized();
    Vec3 r = f.cross(up).normalized();
    Vec3 u = r.cross(f);
    for (int i = 0; i < 16; i++) m[i] = 0.f;
    // Fill column by column
    m[0]=r.x; m[1]=u.x; m[2]=-f.x;
    m[4]=r.y; m[5]=u.y; m[6]=-f.y;
    m[8]=r.z; m[9]=u.z; m[10]=-f.z;
    m[12]=-r.dot(eye);
    m[13]=-u.dot(eye);
    m[14]= f.dot(eye);
    m[15]=1.f;
}

// Perspective projection (column-major)
inline void mat_perspective(float* m, float fovDeg, float aspect, float near, float far) {
    for (int i = 0; i < 16; i++) m[i] = 0.f;
    float f = 1.f / tanf(fovDeg * PI_F / 360.f);
    m[0]  = f / aspect;
    m[5]  = f;
    m[10] = (far + near) / (near - far);
    m[11] = -1.f;
    m[14] = (2.f * far * near) / (near - far);
}

// Convenience: compose T * R * S
inline void mat_trs(float* out,
                    float tx, float ty, float tz,
                    float ry,
                    float sx, float sy, float sz) {
    float T[16], R[16], S[16], tmp[16];
    mat_translate(T, tx, ty, tz);
    mat_rotY(R, ry);
    mat_scale(S, sx, sy, sz);
    mat_mul(tmp, R, S);
    mat_mul(out, T, tmp);
}
