#include "building.h"
#include "shader.h"
#include "math_utils.h"
#include "constants.h"
#include <algorithm>
#include <cmath>

// 5 buildings at positions clearly outside the oval track
// (track outer ellipse approx a+w/2=33.5, b+w/2=21.5)
static const struct {
    float x, z, w, d, h;
    float r, g, b;
    float ar, ag, ab;
    int textureKind;
    int style;
} BDATA[NUM_BUILDINGS] = {
    { -44.f,   0.f, 7.f, 5.f,  7.f,  0.78f,0.44f,0.32f, 0.95f,0.87f,0.72f, 0, 0 },
    {  44.f,   0.f, 7.f, 6.f, 10.f,  0.60f,0.48f,0.32f, 0.29f,0.21f,0.13f, 1, 1 },
    {   0.f, -28.f, 8.f, 6.f, 14.f,  0.63f,0.65f,0.68f, 0.96f,0.75f,0.22f, 2, 2 },
    { -38.f, -24.f, 7.f, 5.f,  7.f,  0.78f,0.80f,0.75f, 0.26f,0.48f,0.33f, 3, 3 },
    {  38.f, -24.f, 8.f, 5.f, 11.f,  0.52f,0.54f,0.60f, 0.18f,0.14f,0.24f, 2, 4 },
};

static void draw_slab_part(const Building& b,
                           const BuildingMeshes& meshes,
                           GLuint shader,
                           float tx, float ty, float tz,
                           float sx, float sy, float sz,
                           float cr, float cg, float cb,
                           float shininess,
                           bool useTexture = false)
{
    float model[16], T[16], S[16];
    mat_translate(T, tx, ty, tz);
    mat_scale(S, sx, sy, sz);
    mat_mul(model, T, S);

    setMat4 (shader, "uModel",      model);
    setVec4 (shader, "uColor",      cr, cg, cb, 1.f);
    setFloat(shader, "uShininess",  shininess);
    setInt  (shader, "uUseTexture", useTexture && b.texture ? 1 : 0);
    if (useTexture && b.texture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, b.texture);
        setInt(shader, "uTex", 0);
    }
    mesh_draw(meshes.slab);
    glBindTexture(GL_TEXTURE_2D, 0);
}

static void draw_window_strip(const Building& b,
                              const BuildingMeshes& meshes,
                              GLuint shader,
                              float y,
                              int count,
                              float width,
                              float height,
                              float faceNX,
                              float faceNZ,
                              float faceOffset,
                              float tint)
{
    float tangentX = -faceNZ;
    float tangentZ =  faceNX;
    float facadeSpan = (fabsf(faceNX) > 0.5f) ? b.depth : b.width;

    for (int w = 0; w < count; w++) {
        float t = count == 1 ? 0.f : ((float)w / (float)(count - 1)) * 2.f - 1.f;
        float wx = b.posX + faceNX * faceOffset + tangentX * t * facadeSpan * 0.32f;
        float wz = b.posZ + faceNZ * faceOffset + tangentZ * t * facadeSpan * 0.32f;
        
        // Window frame (outer border)
        draw_slab_part(b, meshes, shader,
                       wx, y, wz,
                       width, height, 0.06f,
                       0.40f + tint, 0.55f + tint * 0.7f, 0.72f + tint * 0.4f,
                       72.f, false);
        
        // Window frame surround (darker frame)
        draw_slab_part(b, meshes, shader,
                       wx - faceNX * 0.05f, y, wz - faceNZ * 0.05f,
                       width + 0.10f, height + 0.10f, 0.02f,
                       0.14f, 0.16f, 0.20f, 20.f, false);
        
        // Window panes (glass effect with subtle divisions)
        float paneW = width * 0.42f;
        float paneH = height * 0.42f;
        // Left pane
        draw_slab_part(b, meshes, shader,
                       wx - paneW * 0.5f, y, wz,
                       paneW, paneH, 0.04f,
                       0.55f + tint * 0.3f, 0.68f + tint * 0.2f, 0.82f + tint * 0.3f,
                       88.f, false);
        // Right pane
        draw_slab_part(b, meshes, shader,
                       wx + paneW * 0.5f, y, wz,
                       paneW, paneH, 0.04f,
                       0.50f + tint * 0.3f, 0.65f + tint * 0.2f, 0.80f + tint * 0.3f,
                       88.f, false);
        
        // Window sill (horizontal trim below window)
        draw_slab_part(b, meshes, shader,
                       wx, y - height * 0.6f, wz,
                       width + 0.08f, 0.06f, 0.08f,
                       0.25f, 0.20f, 0.14f, 24.f, false);
        
        // Window lintel (horizontal trim above window)
        draw_slab_part(b, meshes, shader,
                       wx, y + height * 0.6f, wz,
                       width + 0.08f, 0.06f, 0.08f,
                       0.25f, 0.20f, 0.14f, 24.f, false);
    }
}

void buildings_init(Building* buildings, int count,
                    BuildingMeshes& meshes,
                    GLuint brickTex, GLuint woodTex,
                    GLuint stoneTex, GLuint plasterTex)
{
    // Unit cube shared mesh
    build_box(meshes.slab, 0.5f, 0.5f, 0.5f); // 1x1x1 box, scale in draw

    for (int i = 0; i < count && i < NUM_BUILDINGS; i++) {
        Building& b = buildings[i];
        b.posX    = BDATA[i].x;
        b.posZ    = BDATA[i].z;
        b.width   = BDATA[i].w;
        b.depth   = BDATA[i].d;
        b.height  = BDATA[i].h;
        b.colorR  = BDATA[i].r;
        b.colorG  = BDATA[i].g;
        b.colorB  = BDATA[i].b;
        b.accentR = BDATA[i].ar;
        b.accentG = BDATA[i].ag;
        b.accentB = BDATA[i].ab;
        b.styleId = BDATA[i].style;
        switch (BDATA[i].textureKind) {
        case 0:  b.texture = brickTex;   break;
        case 1:  b.texture = woodTex;    break;
        case 2:  b.texture = stoneTex;   break;
        default: b.texture = plasterTex; break;
        }
        b.shininess = 16.f;

        float hw = b.width * 0.5f, hd = b.depth * 0.5f;
        b.footprint = { b.posX-hw, b.posX+hw, b.posZ-hd, b.posZ+hd };

        // Direction from building toward track center (origin)
        float dx = -b.posX, dz = -b.posZ;
        float len = sqrtf(dx*dx + dz*dz);
        b.facingX = dx / len;
        b.facingZ = dz / len;
    }
}

void building_draw(const Building& b,
                   const BuildingMeshes& meshes,
                   GLuint shader)
{
    // Main body
    draw_slab_part(b, meshes, shader,
                   b.posX, b.height * 0.5f, b.posZ,
                   b.width, b.height, b.depth,
                   b.colorR, b.colorG, b.colorB,
                   b.shininess, true);

    const bool frontOnX = fabsf(b.facingX) > fabsf(b.facingZ);
    const float frontNX = frontOnX ? (b.facingX > 0.f ? 1.f : -1.f) : 0.f;
    const float frontNZ = frontOnX ? 0.f : (b.facingZ > 0.f ? 1.f : -1.f);
    const float sideNX = -frontNZ;
    const float sideNZ =  frontNX;
    const float frontOffset = frontOnX ? (b.width * 0.5f + 0.03f) : (b.depth * 0.5f + 0.03f);
    const float sideOffset  = frontOnX ? (b.depth * 0.5f + 0.03f) : (b.width * 0.5f + 0.03f);
    const int floors = std::max(1, (int)(b.height / 3.1f));

    // Facade-aligned dimensions so door panels don't appear rotated on X-facing fronts.
    const float doorW  = frontOnX ? 0.08f : 1.15f;
    const float doorD  = frontOnX ? 1.15f : 0.08f;
    const float stepW  = frontOnX ? 0.45f : 1.8f;
    const float stepD  = frontOnX ? 1.8f : 0.45f;
    const float lintelW = frontOnX ? 0.30f : 2.2f;
    const float lintelD = frontOnX ? 2.2f : 0.30f;

    for (int fl = 1; fl < floors; fl++) {
        float y = (b.height / floors) * fl;
        draw_slab_part(b, meshes, shader,
                       b.posX, y, b.posZ,
                       b.width + 0.10f, 0.10f, b.depth + 0.10f,
                       b.accentR * 0.9f, b.accentG * 0.9f, b.accentB * 0.9f, 18.f, false);
    }

    // Front door and step
    draw_slab_part(b, meshes, shader,
                   b.posX + frontNX * frontOffset, 1.15f, b.posZ + frontNZ * frontOffset,
                   doorW, 2.0f, doorD,
                   0.20f, 0.13f, 0.08f, 18.f, false);
    draw_slab_part(b, meshes, shader,
                   b.posX + frontNX * (frontOffset + 0.28f), 0.18f, b.posZ + frontNZ * (frontOffset + 0.28f),
                   stepW, 0.12f, stepD,
                   0.55f, 0.52f, 0.46f, 8.f, false);

    draw_slab_part(b, meshes, shader,
                   b.posX + frontNX * (frontOffset + 0.16f), 2.55f, b.posZ + frontNZ * (frontOffset + 0.16f),
                   lintelW, 0.10f, lintelD,
                   b.accentR * 0.85f, b.accentG * 0.85f, b.accentB * 0.85f, 18.f, false);

    // Style-dependent architectural accents
    switch (b.styleId) {
    case 0: {
        // Brick civic hall: stepped cornice + centered entry massing
        draw_slab_part(b, meshes, shader,
                       b.posX, b.height + 0.35f, b.posZ,
                       b.width + 0.6f, 0.45f, b.depth + 0.6f,
                       b.accentR, b.accentG, b.accentB, 20.f, false);
        draw_slab_part(b, meshes, shader,
                       b.posX, b.height + 0.82f, b.posZ,
                       b.width * 0.74f, 0.34f, b.depth * 0.74f,
                       0.58f, 0.50f, 0.40f, 20.f, false);
        draw_slab_part(b, meshes, shader,
                       b.posX + frontNX * frontOffset, b.height * 0.45f, b.posZ + frontNZ * frontOffset,
                       1.2f, b.height * 0.8f, 0.1f,
                       0.78f, 0.72f, 0.64f, 24.f, false);
        break;
    }
    case 1: {
        // Timber loft: expressed frame and compact roof
        for (int s = -1; s <= 1; s += 2) {
            draw_slab_part(b, meshes, shader,
                           b.posX + frontNX * frontOffset + sideNX * s * (frontOnX ? b.depth * 0.32f : b.width * 0.32f),
                           b.height * 0.52f,
                           b.posZ + frontNZ * frontOffset + sideNZ * s * (frontOnX ? b.depth * 0.32f : b.width * 0.32f),
                           0.4f, b.height * 0.9f, 0.1f,
                           b.accentR, b.accentG, b.accentB, 18.f, false);
        }
        draw_slab_part(b, meshes, shader,
                       b.posX, b.height + 0.55f, b.posZ,
                       b.width + 1.0f, 0.25f, b.depth + 1.2f,
                       0.24f, 0.17f, 0.10f, 18.f, false);
        draw_slab_part(b, meshes, shader,
                       b.posX, b.height + 1.10f, b.posZ,
                       b.width * 0.82f, 0.22f, b.depth * 0.92f,
                       0.18f, 0.13f, 0.08f, 14.f, false);
        break;
    }
    case 2: {
        // Stone tower: strong belt courses and lantern crown
        for (int k = 1; k <= 3; k++) {
            float y = (b.height / 4.f) * k;
            draw_slab_part(b, meshes, shader,
                           b.posX, y, b.posZ,
                           b.width + 0.35f, 0.2f, b.depth + 0.35f,
                           b.accentR, b.accentG, b.accentB, 28.f, false);
        }
        draw_slab_part(b, meshes, shader,
                       b.posX, b.height + 0.7f, b.posZ,
                       b.width * 0.75f, 0.7f, b.depth * 0.75f,
                       0.22f, 0.22f, 0.28f, 30.f, false);
        draw_slab_part(b, meshes, shader,
                       b.posX, b.height + 1.35f, b.posZ,
                       0.85f, 0.65f, 0.85f,
                       0.30f, 0.32f, 0.38f, 28.f, false);
        break;
    }
    case 3: {
        // Plaster courtyard block: glazed edge with green roof band
        draw_slab_part(b, meshes, shader,
                       b.posX, b.height + 0.25f, b.posZ,
                       b.width + 0.45f, 0.18f, b.depth + 0.45f,
                       0.24f, 0.44f, 0.28f, 14.f, false);
        break;
    }
    default: {
        // Stone apartment block: clean balcony band and roof cap
        draw_slab_part(b, meshes, shader,
                       b.posX + frontNX * frontOffset, b.height * 0.35f, b.posZ + frontNZ * frontOffset,
                       b.width * 0.85f, 0.24f, 0.18f,
                       b.accentR, b.accentG, b.accentB, 36.f, false);
        draw_slab_part(b, meshes, shader,
                       b.posX, b.height + 0.42f, b.posZ,
                       b.width + 0.5f, 0.36f, b.depth + 0.5f,
                       0.12f, 0.08f, 0.16f, 22.f, false);
        break;
    }
    }

    // Windows vary with style for stronger visual difference
    int windowsPerFloor = (b.styleId == 2) ? 1 : ((b.styleId == 1 || b.styleId == 4) ? 3 : 2);
    for (int fl = 0; fl < floors; fl++) {
        float wy = 1.8f + fl * 3.0f;
        float ww = (b.styleId == 2) ? 1.1f : 0.72f;
        float wh = (b.styleId == 1) ? 0.55f : 0.65f;
        float tint = 0.05f * (float)((fl + b.styleId) % 3);
        draw_window_strip(b, meshes, shader, wy, windowsPerFloor, ww, wh, frontNX, frontNZ, frontOffset, tint);

        if (b.styleId == 0 || b.styleId == 3) {
            draw_window_strip(b, meshes, shader, wy, 2, 0.56f, 0.56f, -frontNX, -frontNZ, frontOffset, tint * 0.7f);
        }

        if (b.styleId != 2) {
            draw_window_strip(b, meshes, shader, wy, 2, 0.52f, 0.58f, sideNX, sideNZ, sideOffset, tint * 0.6f);
        }
    }
}
