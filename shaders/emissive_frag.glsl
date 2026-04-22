#version 330 core
out vec4 FragColor;

uniform vec4 uColor;   // emissive color passed from spotlight_draw_marker

void main() {
    FragColor = uColor;
}
