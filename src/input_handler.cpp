#include <glad/glad.h>    // must come before GLFW
#include <GLFW/glfw3.h>
#include "input_handler.h"
#include "world.h"
#include "car.h"
#include "camera_system.h"
#include "fan.h"
#include "constants.h"
#include <cmath>

static World*         s_world = nullptr;
static Car*           s_car   = nullptr;
static CameraSystem*  s_cam   = nullptr;
static bool           s_quit  = false;

// GLFW key callback: handles discrete single-press actions
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    (void)scancode;
    if (action == GLFW_PRESS || action == GLFW_REPEAT) {
        switch (key) {
        // Speed
        case GLFW_KEY_F: if (s_car && !s_car->stopped) car_change_speed(*s_car,  CAR_SPEED_INCR); break;
        case GLFW_KEY_S: if (s_car && !s_car->stopped) car_change_speed(*s_car, -CAR_SPEED_INCR); break;

        // Steering (can turn even when stopped)
        case GLFW_KEY_L:
            if (s_car) car_turn(*s_car,  CAR_TURN_DEG * PI_F / 180.f);
            break;
        case GLFW_KEY_R:
            if (s_car) car_turn(*s_car, -CAR_TURN_DEG * PI_F / 180.f);
            break;

        // Fan speed
        case GLFW_KEY_W:
            if (mods & GLFW_MOD_SHIFT) fan_decrease_speed();
            else                       fan_increase_speed();
            break;

        case GLFW_KEY_H:
            if (s_car) car_toggle_headlights(*s_car);
            break;

        case GLFW_KEY_T:
            if (s_world) world_toggle_street_lights(*s_world);
            break;

        case GLFW_KEY_Y:
            if (s_world) world_toggle_storm(*s_world);
            break;

        // Camera modes
        case GLFW_KEY_1: if (s_cam) camera_set_mode(*s_cam, 1); break;
        case GLFW_KEY_2: if (s_cam) camera_set_mode(*s_cam, 2); break;
        case GLFW_KEY_3: if (s_cam) camera_set_mode(*s_cam, 3); break;
        case GLFW_KEY_4: if (s_cam) camera_set_mode(*s_cam, 4); break;
        case GLFW_KEY_5: if (s_cam) camera_set_mode(*s_cam, 5); break;

        // Ground camera look L/R
        case GLFW_KEY_Q: if (s_cam) camera_ground_yaw(*s_cam, -PI_F/18.f); break; // –10°
        case GLFW_KEY_E: if (s_cam) camera_ground_yaw(*s_cam,  PI_F/18.f); break; // +10°

        // Reset world
        case GLFW_KEY_BACKSPACE:
        case GLFW_KEY_ENTER:
            if (s_world) world_reset(*s_world);
            break;

        case GLFW_KEY_ESCAPE: s_quit = true; break;
        default: break;
        }
    }
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int /*mods*/) {
    if (!s_cam || !s_world) return;
    if (button != GLFW_MOUSE_BUTTON_MIDDLE) return;

    double x = 0.0, y = 0.0;
    glfwGetCursorPos(window, &x, &y);
    if (action == GLFW_PRESS) {
        camera_begin_free_look(*s_cam, s_world->car, s_world->spotlights, x, y);
    } else if (action == GLFW_RELEASE) {
        camera_end_free_look_drag(*s_cam);
    }
}

static void cursor_pos_callback(GLFWwindow* /*window*/, double x, double y) {
    if (s_cam) camera_update_free_look_drag(*s_cam, x, y);
}

static void scroll_callback(GLFWwindow* /*window*/, double /*xoffset*/, double yoffset) {
    if (s_cam) camera_zoom_free_look(*s_cam, (float)yoffset);
}

void input_init(GLFWwindow* window, World* world) {
    s_world = world;
    s_car   = world ? &world->car : nullptr;
    s_cam   = world ? &world->camera : nullptr;
    s_quit = false;
    glfwSetKeyCallback(window, key_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetCursorPosCallback(window, cursor_pos_callback);
    glfwSetScrollCallback(window, scroll_callback);
}

void input_process(GLFWwindow* /*window*/, Car* /*car*/) {
    // All discrete; nothing extra needed here currently.
}

bool input_should_quit() { return s_quit; }
