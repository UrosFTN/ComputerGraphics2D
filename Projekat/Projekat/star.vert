#version 330 core

layout(location = 0) in vec2 inPos;
layout(location = 1) in vec3 inCol;
uniform mat4 transform;
out vec3 chCol;

void main(){
	chCol = inCol;
	gl_Position = transform * vec4(inPos, 0.0, 1.0);
}
