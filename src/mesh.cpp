#include "mesh.h"
#include <cmath>
#include "constants.h"

void mesh_upload(Mesh& mesh,
                 const std::vector<float>&        verts,
                 const std::vector<unsigned int>& indices)
{
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float),
                 verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int),
                 indices.data(), GL_STATIC_DRAW);

    // location 0: position (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // location 1: normal (3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // location 2: texcoord (2 floats)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    mesh.count   = (GLsizei)indices.size();
    mesh.indexed = true;
}

void mesh_free(Mesh& mesh) {
    if (mesh.vao) { glDeleteVertexArrays(1, &mesh.vao); mesh.vao = 0; }
    if (mesh.vbo) { glDeleteBuffers(1, &mesh.vbo); mesh.vbo = 0; }
    if (mesh.ebo) { glDeleteBuffers(1, &mesh.ebo); mesh.ebo = 0; }
}

void mesh_draw(const Mesh& mesh) {
    if (!mesh.vao) return;
    glBindVertexArray(mesh.vao);
    if (mesh.indexed)
        glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_INT, nullptr);
    else
        glDrawArrays(GL_TRIANGLES, 0, mesh.count);
    glBindVertexArray(0);
}

// -----------------------------------------------------------------------
// Primitive builders
// -----------------------------------------------------------------------

// Helper: push one vertex (pos, normal, uv) into flat float array
static void push_vertex(std::vector<float>& v,
                        float px, float py, float pz,
                        float nx, float ny, float nz,
                        float u,  float vv)
{
    v.insert(v.end(), {px,py,pz, nx,ny,nz, u,vv});
}

// Helper: add a quad as two triangles given 4 vertex indices
static void push_quad(std::vector<unsigned int>& idx, unsigned int a, unsigned int b,
                      unsigned int c, unsigned int d)
{
    idx.insert(idx.end(), {a,b,c, a,c,d});
}

// build_box: axis-aligned box centered at origin with half-extents hw, hh, hd
void build_box(Mesh& m, float hw, float hh, float hd) {
    std::vector<float>        verts;
    std::vector<unsigned int> inds;

    // Each face: 4 vertices, 6 indices
    auto add_face = [&](float ax,float ay,float az, float bx,float by,float bz,
                        float cx,float cy,float cz, float dx,float dy,float dz,
                        float nx,float ny,float nz) {
        unsigned int base = (unsigned int)(verts.size() / 8);
        push_vertex(verts, ax,ay,az, nx,ny,nz, 0,0);
        push_vertex(verts, bx,by,bz, nx,ny,nz, 1,0);
        push_vertex(verts, cx,cy,cz, nx,ny,nz, 1,1);
        push_vertex(verts, dx,dy,dz, nx,ny,nz, 0,1);
        push_quad(inds, base, base+1, base+2, base+3);
    };

    add_face(-hw,-hh, hd,  hw,-hh, hd,  hw, hh, hd, -hw, hh, hd,  0, 0, 1); // +Z
    add_face( hw,-hh,-hd, -hw,-hh,-hd, -hw, hh,-hd,  hw, hh,-hd,  0, 0,-1); // -Z
    add_face(-hw,-hh,-hd, -hw,-hh, hd, -hw, hh, hd, -hw, hh,-hd, -1, 0, 0); // -X
    add_face( hw,-hh, hd,  hw,-hh,-hd,  hw, hh,-hd,  hw, hh, hd,  1, 0, 0); // +X
    add_face(-hw, hh, hd,  hw, hh, hd,  hw, hh,-hd, -hw, hh,-hd,  0, 1, 0); // +Y top
    add_face(-hw,-hh,-hd,  hw,-hh,-hd,  hw,-hh, hd, -hw,-hh, hd,  0,-1, 0); // -Y bot

    mesh_upload(m, verts, inds);
}

// build_cylinder: axis along Y, centered at origin, total height 2*halfH
void build_cylinder(Mesh& m, float radius, float halfH, int slices) {
    std::vector<float>        verts;
    std::vector<unsigned int> inds;

    float step = 2.f * PI_F / (float)slices;

    // Side vertices: 2 rings (bottom + top) each with slices+1 vertices
    for (int i = 0; i <= slices; i++) {
        float a  = i * step;
        float ca = cosf(a), sa = sinf(a);
        float nx = ca, nz = sa; // outward normal
        float u  = (float)i / slices;
        push_vertex(verts, ca*radius, -halfH, sa*radius, nx,0,nz, u,0); // bottom
        push_vertex(verts, ca*radius,  halfH, sa*radius, nx,0,nz, u,1); // top
    }
    // Side quads
    for (int i = 0; i < slices; i++) {
        unsigned int b0 = 2*i, t0 = 2*i+1, b1 = 2*(i+1), t1 = 2*(i+1)+1;
        push_quad(inds, b0, b1, t1, t0);
    }

    // Cap centers
    unsigned int botCenter = (unsigned int)(verts.size() / 8);
    push_vertex(verts, 0,-halfH,0, 0,-1,0, 0.5f,0.5f);
    unsigned int topCenter = (unsigned int)(verts.size() / 8);
    push_vertex(verts, 0, halfH,0, 0, 1,0, 0.5f,0.5f);

    // Cap rim vertices
    unsigned int rimBottom = (unsigned int)(verts.size() / 8);
    for (int i = 0; i <= slices; i++) {
        float a  = i * step;
        float ca = cosf(a), sa = sinf(a);
        float u  = 0.5f + 0.5f*ca, v = 0.5f + 0.5f*sa;
        push_vertex(verts, ca*radius,-halfH,sa*radius, 0,-1,0, u,v);
    }
    unsigned int rimTop = (unsigned int)(verts.size() / 8);
    for (int i = 0; i <= slices; i++) {
        float a  = i * step;
        float ca = cosf(a), sa = sinf(a);
        float u  = 0.5f + 0.5f*ca, v = 0.5f + 0.5f*sa;
        push_vertex(verts, ca*radius, halfH,sa*radius, 0, 1,0, u,v);
    }

    // Bottom cap triangles (CW to face down)
    for (int i = 0; i < slices; i++)
        inds.insert(inds.end(), {botCenter, rimBottom+i+1, rimBottom+i});
    // Top cap triangles (CCW to face up)
    for (int i = 0; i < slices; i++)
        inds.insert(inds.end(), {topCenter, rimTop+i, rimTop+i+1});

    mesh_upload(m, verts, inds);
}

// build_oval_track: horizontal (XZ) ring between two ellipses.
// a = semi-major (X), b = semi-minor (Z), width = road width
void build_oval_track(Mesh& m, float a, float b, float width, int segs) {
    std::vector<float>        verts;
    std::vector<unsigned int> inds;

    float ht = width * 0.5f;
    float ai = a - ht, bi = b - ht; // inner ellipse
    float ao = a + ht, bo = b + ht; // outer ellipse

    for (int i = 0; i <= segs; i++) {
        float t  = 2.f * PI_F * i / segs;
        float ct = cosf(t), st = sinf(t);
        float xi = ai*ct, zi = bi*st;
        float xo = ao*ct, zo = bo*st;
        float u  = (float)i / segs;
        push_vertex(verts, xi, 0, zi, 0,1,0, u, 0); // inner
        push_vertex(verts, xo, 0, zo, 0,1,0, u, 1); // outer
    }

    for (int i = 0; i < segs; i++) {
        unsigned int i0=2*i, i1=2*i+1, i2=2*(i+1), i3=2*(i+1)+1;
        push_quad(inds, i0, i2, i3, i1);
    }
    mesh_upload(m, verts, inds);
}

// build_ground_plane: flat XZ plane centered at origin with repeatable UVs.
void build_ground_plane(Mesh& m, float halfW, float halfD, float uRepeat, float vRepeat) {
    std::vector<float> verts;
    std::vector<unsigned int> inds;

    // y=0 plane, normal up.
    push_vertex(verts, -halfW, 0.f, -halfD, 0.f, 1.f, 0.f, 0.f,      0.f);
    push_vertex(verts,  halfW, 0.f, -halfD, 0.f, 1.f, 0.f, uRepeat,  0.f);
    push_vertex(verts,  halfW, 0.f,  halfD, 0.f, 1.f, 0.f, uRepeat,  vRepeat);
    push_vertex(verts, -halfW, 0.f,  halfD, 0.f, 1.f, 0.f, 0.f,      vRepeat);

    push_quad(inds, 0, 1, 2, 3);
    mesh_upload(m, verts, inds);
}

// build_sphere: UV sphere of given radius
void build_sphere(Mesh& m, float radius, int stacks, int slices) {
    std::vector<float>        verts;
    std::vector<unsigned int> inds;

    for (int st = 0; st <= stacks; st++) {
        float phi = PI_F * st / stacks;  // 0 .. PI
        float cp  = cosf(phi), sp = sinf(phi);
        for (int sl = 0; sl <= slices; sl++) {
            float theta = 2.f * PI_F * sl / slices;
            float ct = cosf(theta), st2 = sinf(theta);
            float x = sp*ct, y = cp, z = sp*st2;
            float u = (float)sl/slices, v = (float)st/stacks;
            push_vertex(verts, x*radius, y*radius, z*radius, x,y,z, u,v);
        }
    }

    for (int st = 0; st < stacks; st++)
        for (int sl = 0; sl < slices; sl++) {
            unsigned int a = st*(slices+1)+sl,   b = a+1;
            unsigned int c = (st+1)*(slices+1)+sl, d = c+1;
            push_quad(inds, a, b, d, c);
        }

    mesh_upload(m, verts, inds);
}

// build_wall_quad: a flat plane for a single wall piece, standing along XY,
// centered at origin, size = width(X) × height(Y), depth along Z = depth.
void build_wall_quad(Mesh& m, float width, float height, float depth) {
    build_box(m, width*0.5f, height*0.5f, depth*0.5f);
}
