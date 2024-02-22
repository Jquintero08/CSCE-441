#version 330 core

in vec3 fragPos;
in vec3 normal;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;

const vec3 lightPos = vec3(1, 1, 1); //Light pos
const vec3 lightColor = vec3(1, 1, 1); //white color

out vec4 fragColor;

void main() {
    vec3 N = normalize(normal);
    vec3 L = normalize(lightPos - fragPos);
    vec3 V = normalize(-fragPos);
    vec3 H = normalize(L + V);

    vec3 ambient = ka * lightColor;

    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = kd * diff * lightColor;

    float spec = pow(max(dot(N, H), 0.0), shininess);
    vec3 specular = ks * spec * lightColor;

    vec3 color = ambient + diffuse + specular;
    fragColor = vec4(color, 1.0);
}
