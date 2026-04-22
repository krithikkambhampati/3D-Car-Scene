#pragma once
// ============================================================
//  input_handler.h  –  GLFW keyboard callback and state.
//  We forward-declare GLFWwindow to avoid pulling in GLFW's
//  OpenGL header before glad.
// ============================================================
struct GLFWwindow;

// Forward declarations
struct Car;
struct CameraSystem;
struct World;

// Pointers set once at init; callback reads through them.
void input_init(GLFWwindow* window, World* world);

// Call each frame to handle continuous key presses
// (held keys for smooth steering are handled here).
// Discrete presses are handled in the GLFW callback set by input_init.
void input_process(GLFWwindow* window, Car* car);

// Should the application exit?
bool input_should_quit();
