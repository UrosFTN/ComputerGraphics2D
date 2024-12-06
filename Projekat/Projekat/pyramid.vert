#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inCol;
uniform vec2 uPos;

out vec3 chCol;

void main()
{
	chCol = inCol;
	gl_Position = vec4(inPos + uPos, 0.0, 1.0);
}