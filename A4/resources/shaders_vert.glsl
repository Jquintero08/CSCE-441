#version 330 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNor;

uniform mat4 MV;
uniform mat4 P;
uniform mat3 invTransformMV;

out vec3 fragPos;
out vec3 normal;

void main() {
    fragPos = vec3(MV * vec4(aPos, 1.0));
    normal = normalize(invTransformMV * aNor);
    gl_Position = P * MV * vec4(aPos, 1.0);
}
