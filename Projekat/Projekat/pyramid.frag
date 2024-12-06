#version 330 core

in vec3 chCol;
out vec4 outCol;

uniform int uW;
uniform int uH;

uniform float time; 


void main()
{
	outCol = vec4(1.0, 0.0, 0.0, 1.0);

	if (mod(gl_FragCoord.x, 10) < 6)
		outCol = vec4(chCol, 1.0);
}