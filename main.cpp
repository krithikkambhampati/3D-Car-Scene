// ============================================================
//  main.cpp  –  Program entry point.
//  Creates an OpenGL 3.3 core context window, then hands off
//  to World for all scene management.
// ============================================================
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <cstring>
#include <string>

#include "src/world.h"
#include "src/input_handler.h"

static const int  WIN_W = 1280, WIN_H = 720;
static const char WIN_TITLE[] = "Assignment 3 – Hierarchical Scene";
static const char WIN_TITLE_REPLAY[] = "Assignment 3 – Hierarchical Scene  [REPLAY MODE]";

// GLFW framebuffer resize callback
static void framebuffer_size_callback(GLFWwindow*, int w, int h) {
    glViewport(0, 0, w, h);
}

int main() {
    // ----- GLFW init -----
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4); // MSAA

    GLFWwindow* window = glfwCreateWindow(WIN_W, WIN_H, WIN_TITLE, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate(); return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // ----- GLAD load -----
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to load OpenGL (GLAD)\n";
        glfwTerminate(); return -1;
    }
    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";

    // ----- OpenGL global state -----
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);  // proper depth testing
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // ----- World + Input init -----
    World world;
    world_init(world);
    input_init(window, &world);

    // Print controls to console
    std::cout << "\n=== Controls ===\n"
              << "  f / s          : increase / decrease car speed\n"
              << "  l / r          : turn car left / right\n"
              << "  w / Shift+W    : increase / decrease fan speed\n"
              << "  h              : toggle car headlights\n"
              << "  t              : toggle street lights\n"
              << "  y              : toggle thunderstorm mode\n"
              << "  1-5            : switch camera (sky/car/ground/light/heli)\n"
              << "  q / e          : ground-cam look left / right\n"
              << "  middle mouse   : free-look orbit camera\n"
              << "  mouse wheel    : zoom free-look camera\n"
              << "  Backspace      : reset world\n"
              << "  Esc            : quit\n"
              << "================\n\n";

    // ----- Main loop -----
    double prevTime = glfwGetTime();
    while (!glfwWindowShouldClose(window) && !input_should_quit()) {
        double now = glfwGetTime();
        float  dt  = (float)(now - prevTime);
        if (dt > 0.05f) dt = 0.05f; // cap at 20 FPS equiv to avoid big jumps
        prevTime = now;

        glfwPollEvents();

        int fbW, fbH;
        glfwGetFramebufferSize(window, &fbW, &fbH);
        if (fbH == 0) fbH = 1;

        world_update(world, dt);
        glfwSetWindowTitle(window, world.replayActive ? WIN_TITLE_REPLAY : WIN_TITLE);

        float clearR, clearG, clearB;
        world_clear_color(world, clearR, clearG, clearB);
        glClearColor(clearR, clearG, clearB, 1.0f);
        glViewport(0, 0, fbW, fbH);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        world_render(world, fbW, fbH);

        glfwSwapBuffers(window);
    }

    world_cleanup(world);
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
