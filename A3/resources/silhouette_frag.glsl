#version 330 core

in vec3 fragPos;
in vec3 normal;

uniform vec3 outlineColor;
uniform float outlineWidth;

out vec4 fragColor;

void main() {
    if (dot(normal, normalize(-fragPos)) < outlineWidth) {
        fragColor = vec4(outlineColor, 1);
    } else {
        fragColor = vec4(1, 1, 1, 1);
    }
}
