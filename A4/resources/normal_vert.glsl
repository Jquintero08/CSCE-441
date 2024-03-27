#version 120

uniform mat4 P;
uniform mat4 MV;

attribute vec4 aPos;
attribute vec3 aNor;


varying vec3 color;


void main()
{
	gl_Position = P * (MV * aPos);
	color = 0.5 * aNor + vec3(0.5, 0.5, 0.5);
}
