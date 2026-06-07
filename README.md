# 3D Car Scene in Open GL

This is an OpenGL 3.3 computer graphics project that renders a hierarchical city-road scene with a controllable car, dynamic lighting, weather effects, and multiple camera modes.

## What the project includes

- A drivable car with speed, steering, headlights, and crash replay behavior
- Multiple camera modes: sky, car, ground, light source, and helicopter views
- Procedural textures and lit 3D scene objects such as buildings, trees, streetlights, and walls
- Storm mode with lightning, flicker, and reduced visibility effects
- Mouse-based free-look orbit camera support

## Build and run

```bash
make clean
make
./app3d
```

## Controls

| Key | Action |
|-----|--------|
| **F** | Increase speed |
| **S** | Decrease speed |
| **L** | Turn left |
| **R** | Turn right |
| **W** | Increase fan speed |
| **Shift+W** | Decrease fan speed |
| **H** | Toggle car headlights |
| **T** | Toggle street lights |
| **B** | Toggle building window lights |
| **Y** | Toggle thunderstorm mode |
| **1–5** | Switch camera mode |
| **Q / E** | Rotate the ground camera left / right |
| **Backspace** | Reset the world |
| **Esc** | Quit the application |

### Mouse controls

- **Middle mouse drag** — enter free-look orbit camera mode
- **Mouse wheel** — zoom in or out in free-look mode

## Repository layout

- `main.cpp` — OpenGL setup and application loop
- `src/` — scene objects, camera logic, collision, rendering, and input handling
- `shaders/` — GLSL shader programs
- `assets/` — textures and other supporting assets
- `glad/` — GLAD loader source used by the build

## Notes

- The window title changes to show replay mode after a crash.
- The scene uses shared world state for lighting, weather, and camera behavior.
- Build output is the `app3d` executable; it should not be committed.