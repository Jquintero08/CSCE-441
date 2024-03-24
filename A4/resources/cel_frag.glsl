#version 330 core

in vec3 fragPos;
in vec3 normal;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;

uniform vec3 lightPositions[2];
uniform vec3 lightColors[2];

out vec4 fragColor;

float quantize(float channel) {
    if (channel < 0.25) {
        return 0.0;
    } else if (channel < 0.5) {
        return 0.25;
    } else if (channel < 0.75) {
        return 0.5;
    } else if (channel < 1.0) {
        return 0.75;
    } else {
        return 1.0;
    }
}

void main() {
    vec3 N = normalize(normal);
    vec3 V = normalize(-fragPos);
    vec3 resultColor = vec3(0.0);

    for(int i = 0; i < 2; ++i) {
        vec3 L = normalize(lightPositions[i] - fragPos);
        vec3 H = normalize(L + V);

        vec3 ambient = ka * lightColors[i];
        vec3 diffuse = kd * max(dot(N, L), 0.0) * lightColors[i];
        vec3 specular = ks * pow(max(dot(N, H), 0.0), shininess) * lightColors[i];

        resultColor += ambient + diffuse + specular;
    }

    resultColor.r = quantize(resultColor.r);
    resultColor.g = quantize(resultColor.g);
    resultColor.b = quantize(resultColor.b);

    if (dot(N, V) < 0.3) {
        resultColor = vec3(0.0);
    }

    fragColor = vec4(resultColor, 1.0);
}
