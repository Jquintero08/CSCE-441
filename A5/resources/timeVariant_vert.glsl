#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTex;

uniform mat4 MV;
uniform mat4 P;

uniform mat3 invTransformMV;
uniform float t;

out vec3 fragPos;
out vec3 normal;
out vec2 TexCoords;


void main() {
    float x = aPos.x;
    float theta = aPos.y;

    vec3 p = vec3(x, (cos(x + t) + 2.0) * cos(theta), (cos(x + t) + 2.0) * sin(theta));
    vec4 p_cam = MV * vec4(p, 1.0);

    vec3 T1 = vec3(1.0, -sin(x + t) * cos(theta), -sin(x + t) * sin(theta));
    vec3 T2 = vec3(0.0, -(cos(x + t) + 2.0) * sin(theta), (cos(x + t) + 2.0) * cos(theta));
    vec3 n = cross(T1, T2);
    
    normal = -normalize(invTransformMV * n);

    fragPos = vec3(p_cam);
    TexCoords = aTex;

    gl_Position = P * p_cam;
}
