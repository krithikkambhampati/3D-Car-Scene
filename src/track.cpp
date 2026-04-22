#include "track.h"
#include "shader.h"
#include "constants.h"
#include "math_utils.h"

void track_init(Track& t, GLuint roadTex, GLuint groundTex) {
    t.roadTex   = roadTex;
    t.groundTex = groundTex;

    // Road ring: between inner (a-w/2, b-w/2) and outer (a+w/2, b+w/2) ellipses
    build_oval_track(t.roadMesh, TRACK_A, TRACK_B, TRACK_WIDTH, TRACK_SEGS);
    build_oval_track(t.laneStripeMesh, TRACK_A, TRACK_B, 0.42f, TRACK_SEGS);
    build_oval_track(t.curbInnerMesh, TRACK_A - TRACK_WIDTH * 0.5f - 0.45f,
                     TRACK_B - TRACK_WIDTH * 0.5f - 0.45f, 0.90f, TRACK_SEGS);
    build_oval_track(t.curbOuterMesh, TRACK_A + TRACK_WIDTH * 0.5f + 0.45f,
                     TRACK_B + TRACK_WIDTH * 0.5f + 0.45f, 0.90f, TRACK_SEGS);

    // Flat ground: a large XZ quad covering the whole arena foot
    build_ground_plane(t.groundMesh, ARENA_HALF_W, ARENA_HALF_D, 18.0f, 12.0f);
}

void track_draw(const Track& t, GLuint shader) {
    float groundModel[16], roadModel[16], stripeModel[16], curbInnerModel[16], curbOuterModel[16];
    mat_translate(groundModel, 0.f, TRACK_GROUND_Y, 0.f);
    mat_translate(roadModel,   0.f, TRACK_ROAD_Y, 0.f);
    mat_translate(stripeModel, 0.f, TRACK_STRIPE_Y, 0.f);
    mat_translate(curbInnerModel, 0.f, TRACK_CURB_INNER_Y, 0.f);
    mat_translate(curbOuterModel, 0.f, TRACK_CURB_OUTER_Y, 0.f);

    // Ground plane sits clearly below the road to avoid depth fighting.
    setMat4 (shader, "uModel",      groundModel);
    setVec4 (shader, "uColor",      0.56f, 0.88f, 0.42f, 1.f);
    setFloat(shader, "uShininess",  6.0f);
    setInt  (shader, "uUseTexture", t.groundTex ? 1 : 0);
    if (t.groundTex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, t.groundTex);
        setInt(shader, "uTex", 0);
    }
    mesh_draw(t.groundMesh);
    glBindTexture(GL_TEXTURE_2D, 0);

    setMat4 (shader, "uModel",      roadModel);
    setVec4 (shader, "uColor",      0.18f, 0.19f, 0.21f, 1.f);
    setFloat(shader, "uShininess",  14.5f);
    setInt  (shader, "uUseTexture", t.roadTex ? 1 : 0);
    if (t.roadTex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, t.roadTex);
        setInt(shader, "uTex", 0);
    }
    mesh_draw(t.roadMesh);
    glBindTexture(GL_TEXTURE_2D, 0);

    setMat4 (shader, "uModel", stripeModel);
    setVec4 (shader, "uColor", 1.00f, 0.90f, 0.18f, 1.f);
    setFloat(shader, "uShininess", 18.f);
    setInt  (shader, "uUseTexture", 0);
    mesh_draw(t.laneStripeMesh);

    setMat4 (shader, "uModel", curbInnerModel);
    setVec4 (shader, "uColor", 0.90f, 0.90f, 0.92f, 1.f);
    setFloat(shader, "uShininess", 4.f);
    setInt  (shader, "uUseTexture", 0);
    mesh_draw(t.curbInnerMesh);

    setMat4 (shader, "uModel", curbOuterModel);
    setVec4 (shader, "uColor", 0.76f, 0.78f, 0.80f, 1.f);
    setFloat(shader, "uShininess", 4.f);
    setInt  (shader, "uUseTexture", 0);
    mesh_draw(t.curbOuterMesh);
}
