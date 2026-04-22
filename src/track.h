#pragma once
#include <glad/glad.h>
#include "mesh.h"
// ============================================================
//  track.h  –  Oval road track.
// ============================================================
struct Track {
    Mesh roadMesh;        // main asphalt ring
    Mesh groundMesh;      // flat ground inside/around track
    Mesh laneStripeMesh;  // center marking ring
    Mesh curbInnerMesh;   // inner curb band
    Mesh curbOuterMesh;   // outer curb band
    GLuint roadTex  = 0;
    GLuint groundTex = 0;
};

void track_init (Track& t, GLuint roadTex, GLuint groundTex);
void track_draw (const Track& t, GLuint shader);
