#version 330 core

in vec3 fragPos;
in vec3 normal;
in vec2 TexCoords;




//uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform vec3 ke;
uniform float shininess;
uniform int numLights;
uniform sampler2D texture0;

struct Light {
    vec3 position;
    vec3 color;
};
uniform Light lights[10];

out vec4 fragColor;

void main() {
    vec3 N = normalize(normal);
    vec3 V = normalize(-fragPos);

    vec3 texColor = texture(texture0, TexCoords).rgb;

    vec3 ambient = ke;
    vec3 result = ambient * texColor;



    const float A0 = 1.0;
    const float A1 = 0.0429;
    const float A2 = 0.9857;

    
    for (int i = 0; i < numLights; i++) {
        vec3 L = normalize(lights[i].position - fragPos);
        vec3 H = normalize(L + V);
        float distance = length(lights[i].position - fragPos);

        //vec3 ambient = ka * lightColor;
        vec3 diffuse = kd * max(dot(N, L), 0.0);
        vec3 specular = ks * pow(max(dot(N, H), 0.0), shininess);

        float attenuation = 1.0 / (A0 + A1 * distance + A2 * distance * distance);

        result += (diffuse + specular) * lights[i].color * attenuation;
    }
    

    fragColor = vec4(result, 1.0);
}