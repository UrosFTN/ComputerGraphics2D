#version 330 core

in vec3 fragColor; // Interpolated color from vertex shader
out vec4 FragColor; // Final color output

void main() {
    FragColor = vec4(0.2,0.2,1.0, 1.0); // Blue color
}