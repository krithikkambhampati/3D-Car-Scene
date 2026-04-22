#include "texture.h"
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <cmath>
#include <cctype>

// Upload a pixel buffer to OpenGL and return a texture ID.
static GLuint upload_texture(const std::vector<unsigned char>& data, int w, int h) {
    GLuint tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, data.data());
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);
    return tex;
}

static bool ppm_read_token(std::ifstream& f, std::string& out) {
    out.clear();
    char ch = 0;
    while (f.get(ch)) {
        if (ch == '#') {
            std::string discard;
            std::getline(f, discard);
            continue;
        }
        if (!std::isspace((unsigned char)ch)) {
            out.push_back(ch);
            break;
        }
    }
    if (out.empty()) return false;
    while (f.get(ch) && !std::isspace((unsigned char)ch)) {
        out.push_back(ch);
    }
    return true;
}

static GLuint load_ppm_texture(const char* path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return 0;

    std::string tok;
    if (!ppm_read_token(f, tok) || tok != "P6") return 0;
    if (!ppm_read_token(f, tok)) return 0;
    int w = std::stoi(tok);
    if (!ppm_read_token(f, tok)) return 0;
    int h = std::stoi(tok);
    if (!ppm_read_token(f, tok)) return 0;
    int maxv = std::stoi(tok);
    if (w <= 0 || h <= 0 || maxv <= 0) return 0;

    std::vector<unsigned char> data((size_t)w * (size_t)h * 3);
    f.read(reinterpret_cast<char*>(data.data()), (std::streamsize)data.size());
    if (!f) return 0;

    if (maxv != 255) {
        for (unsigned char& c : data) {
            c = (unsigned char)std::max(0, std::min(255, (int)c * 255 / maxv));
        }
    }

    return upload_texture(data, w, h);
}

// Brick texture: red/orange bricks separated by gray mortar lines.
GLuint gen_brick_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/building_brick_diff.ppm")) return t;
    const int W = 256, H = 256;
    const int BRICK_W = 64, BRICK_H = 28, MORTAR = 4;
    std::vector<unsigned char> pixels(W * H * 3);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            // Alternate rows are offset by half a brick width
            int row    = y / (BRICK_H + MORTAR);
            int offset = (row % 2) ? BRICK_W / 2 : 0;
            int lx     = (x + offset) % (BRICK_W + MORTAR);
            int ly     = y % (BRICK_H + MORTAR);

            bool mortar = (lx >= BRICK_W) || (ly >= BRICK_H);
            unsigned char r, g, b;
            if (mortar) {
                r = 160; g = 160; b = 160; // gray mortar
            } else {
                // Brick color with slight variation
                int noise = ((x * 7 + y * 13) % 20) - 10;
                r = (unsigned char)(180 + noise);
                g = (unsigned char)(80  + noise / 2);
                b = (unsigned char)(60  + noise / 4);
            }
            int idx = (y * W + x) * 3;
            pixels[idx]=r; pixels[idx+1]=g; pixels[idx+2]=b;
        }
    }
    return upload_texture(pixels, W, H);
}

// Wood texture: horizontal grain bands of varying brown.
GLuint gen_wood_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/building_wood_diff.ppm")) return t;
    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            // Concentric ring-like grain using a sine pattern
            float grain = sinf(y * 0.25f + x * 0.04f) * 20.f;
            int noise   = ((x * 3 + y * 11) % 10) - 5;
            unsigned char r = (unsigned char)(160 + grain + noise);
            unsigned char g = (unsigned char)(100 + grain/2 + noise/2);
            unsigned char b = (unsigned char)(55  + noise/4);
            // Clamp
            if (r > 220) r = 220;
            if (r < 100) r = 100;
            if (g > 150) g = 150;
            if (g < 60)  g = 60;
            int idx = (y * W + x) * 3;
            pixels[idx]=r; pixels[idx+1]=g; pixels[idx+2]=b;
        }
    }
    return upload_texture(pixels, W, H);
}

GLuint gen_stone_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/building_concrete_diff.ppm")) return t;
    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int blockX = x / 32;
            bool seam = (x % 32) < 3 || (y % 22) < 3;
            int noise = ((x * 17 + y * 9 + blockX * 13) % 28) - 14;
            int fleck = ((x * 11 + y * 5) % 23 == 0) ? 18 : 0;

            int r = 126 + noise + fleck;
            int g = 128 + noise / 2 + fleck;
            int b = 132 + noise / 3 + fleck;
            if (seam) {
                r = 96; g = 98; b = 102;
            }

            r = std::max(82, std::min(178, r));
            g = std::max(84, std::min(182, g));
            b = std::max(88, std::min(188, b));

            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)r;
            pixels[idx + 1] = (unsigned char)g;
            pixels[idx + 2] = (unsigned char)b;
        }
    }
    return upload_texture(pixels, W, H);
}

GLuint gen_plaster_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/building_concrete_diff.ppm")) return t;
    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int noise = ((x * 7 + y * 19 + (x / 9) * 13) % 26) - 13;
            int warm = ((x / 37 + y / 29) % 2) ? 6 : -4;
            int r = 214 + noise + warm;
            int g = 206 + noise / 2 + warm;
            int b = 192 + noise / 3;

            if (((x + 2 * y) % 41) == 0) {
                r -= 10; g -= 12; b -= 8;
            }

            r = std::max(176, std::min(236, r));
            g = std::max(168, std::min(228, g));
            b = std::max(154, std::min(214, b));

            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)r;
            pixels[idx + 1] = (unsigned char)g;
            pixels[idx + 2] = (unsigned char)b;
        }
    }
    return upload_texture(pixels, W, H);
}

// Road texture: dark asphalt with a faint center line and shoulder wear.
GLuint gen_road_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/road_asphalt_diff.ppm")) return t;
    const int W = 256, H = 128;
    std::vector<unsigned char> pixels(W * H * 3);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int noise = ((x * 5 + y * 7 + (x / 13) * 11) % 28) - 14;
            float vCoord = (float)y / (float)(H - 1);
            float laneBand = fabsf(vCoord - 0.5f);
            bool centerStripe = laneBand < 0.024f && ((x / 20) % 2 == 0);
            bool shoulder = (vCoord < 0.10f || vCoord > 0.90f);
            bool tireWear = fabsf(vCoord - 0.34f) < 0.05f || fabsf(vCoord - 0.66f) < 0.05f;
            int base = 48 + noise;
            if (shoulder) base += 11;
            if (tireWear) base += 6;
            if (((x + 3 * y) % 53) == 0) base += 16;
            int r = base;
            int g = base + 2;
            int b = base + 4;
            if (centerStripe) {
                r = 214 + noise / 3;
                g = 198 + noise / 4;
                b = 92 + noise / 6;
            }
            r = std::max(26, std::min(230, r));
            g = std::max(28, std::min(222, g));
            b = std::max(30, std::min(160, b));
            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)r;
            pixels[idx+1] = (unsigned char)g;
            pixels[idx+2] = (unsigned char)b;
        }
    }
    return upload_texture(pixels, W, H);
}

GLuint gen_grass_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/grass_ground_diff.ppm")) return t;
    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int noise = ((x * 13 + y * 17 + (y / 7) * 19) % 34) - 17;
            int blade = ((x + 3 * y) % 29 < 3) ? 18 : 0;
            int patch = (((x / 26) + (y / 22)) % 2) ? 10 : -6;
            int r = 44 + patch / 2 + noise / 3;
            int g = 98 + patch + noise + blade;
            int b = 40 + noise / 4;

            if (((x - y) % 47) == 0) {
                r += 12; g += 10; b += 4;
            }

            r = std::max(30, std::min(82, r));
            g = std::max(66, std::min(152, g));
            b = std::max(24, std::min(72, b));

            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)r;
            pixels[idx + 1] = (unsigned char)g;
            pixels[idx + 2] = (unsigned char)b;
        }
    }
    return upload_texture(pixels, W, H);
}

GLuint gen_bark_texture() {
    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int ridge = ((x / 9) % 2 == 0) ? 10 : -8;
            int noise = ((x * 5 + y * 17 + (y / 13) * 9) % 26) - 13;
            int crack = ((x + y * 3) % 47 < 2) ? -18 : 0;
            int r = 98 + ridge + noise + crack;
            int g = 66 + ridge / 2 + noise / 2 + crack / 2;
            int b = 38 + noise / 3;

            r = std::max(58, std::min(138, r));
            g = std::max(36, std::min(92, g));
            b = std::max(20, std::min(56, b));

            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)r;
            pixels[idx + 1] = (unsigned char)g;
            pixels[idx + 2] = (unsigned char)b;
        }
    }
    return upload_texture(pixels, W, H);
}

GLuint gen_leaf_texture() {
    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int noise = ((x * 11 + y * 7 + (x / 15) * 13) % 34) - 17;
            int cluster = (((x / 18) + (y / 18)) % 2) ? 12 : -8;
            int vein = ((x + 2 * y) % 31 < 3) ? 14 : 0;
            int r = 36 + cluster / 2 + noise / 3;
            int g = 112 + cluster + noise + vein;
            int b = 34 + noise / 4;

            r = std::max(20, std::min(74, r));
            g = std::max(74, std::min(166, g));
            b = std::max(18, std::min(62, b));

            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)r;
            pixels[idx + 1] = (unsigned char)g;
            pixels[idx + 2] = (unsigned char)b;
        }
    }
    return upload_texture(pixels, W, H);
}

GLuint gen_car_paint_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/car_blue_metal_diff.ppm")) return t;

    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int n = ((x * 9 + y * 13 + (x / 7) * 5) % 24) - 12;
            int r = 46 + n / 4;
            int g = 86 + n / 3;
            int b = 156 + n;
            r = std::max(24, std::min(80, r));
            g = std::max(58, std::min(126, g));
            b = std::max(110, std::min(210, b));
            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)r;
            pixels[idx + 1] = (unsigned char)g;
            pixels[idx + 2] = (unsigned char)b;
        }
    }
    return upload_texture(pixels, W, H);
}

GLuint gen_car_metal_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/car_metal_diff.ppm")) return t;

    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int streak = (int)(12.f * sinf((float)x * 0.22f));
            int n = ((x * 7 + y * 11) % 20) - 10;
            int r = 122 + streak + n / 2;
            int g = 126 + streak + n / 2;
            int b = 132 + streak + n / 2;
            r = std::max(86, std::min(178, r));
            g = std::max(90, std::min(182, g));
            b = std::max(96, std::min(188, b));
            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)r;
            pixels[idx + 1] = (unsigned char)g;
            pixels[idx + 2] = (unsigned char)b;
        }
    }
    return upload_texture(pixels, W, H);
}

GLuint gen_car_rubber_texture() {
    if (GLuint t = load_ppm_texture("assets/textures/car_rubber_diff.ppm")) return t;

    const int W = 256, H = 256;
    std::vector<unsigned char> pixels(W * H * 3);
    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            int tread = (((x / 8) + (y / 8)) % 2) ? 8 : -8;
            int n = ((x * 13 + y * 5) % 18) - 9;
            int v = 46 + tread + n;
            v = std::max(22, std::min(72, v));
            int idx = (y * W + x) * 3;
            pixels[idx] = (unsigned char)v;
            pixels[idx + 1] = (unsigned char)v;
            pixels[idx + 2] = (unsigned char)(v + 2);
        }
    }
    return upload_texture(pixels, W, H);
}
