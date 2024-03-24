#version 330 core

in vec3 fragPos;
in vec3 normal;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;

uniform vec3 lightPositions[2];
uniform vec3 lightColors[2];
uniform bool lightEnabled[2];

const vec3 lightColor = vec3(1, 1, 1); 

out vec4 fragColor;

void main() {
    vec3 N = normalize(normal);
    vec3 V = normalize(-fragPos);
    
    vec3 resultColor = vec3(0.0);

    for(int i = 0; i < 2; ++i) {
        if(!lightEnabled[i]){
            continue; //Check if light enabled
        }
        vec3 L = normalize(lightPositions[i] - fragPos);
        vec3 H = normalize(L + V);

        vec3 ambient = ka * lightColors[i];
        vec3 diffuse = kd * max(dot(N, L), 0.0) * lightColors[i];
        vec3 specular = ks * pow(max(dot(N, H), 0.0), shininess) * lightColors[i];

        resultColor += ambient + diffuse + specular;
    }

    fragColor = vec4(resultColor, 1.0);
}