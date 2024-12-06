#version 330 core

layout(location = 0) in vec2 aPos;  // Position of the vertex
layout(location = 1) in vec3 aColor; // The color passed to the shader

out vec3 chCol; // Color to be passed to the fragment shader

uniform vec2 uPos; // The fish's position

void main()
{
    // Apply translation for fish movement
    vec2 pos = aPos + uPos;

    // Set the final position in clip space
    gl_Position = vec4(pos, 0.0, 1.0);

    // Pass color to the fragment shader
    chCol = aColor;
}
