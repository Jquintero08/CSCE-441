#version 330 core

in vec3 fragPos;
in vec3 normal;
in vec2 vTex;


uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float shininess;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform sampler2D groundTexture;



const vec3 lightColor = vec3(1, 1, 1); 

out vec4 fragColor;

void main() {
    vec3 N = normalize(normal);
    vec3 V = normalize(-fragPos);
    
    vec3 L = normalize(lightPos - fragPos);
    vec3 H = normalize(L + V);

    vec4 texColor = texture(groundTexture, vTex);
    vec3 ambient = ka * lightColor;
    vec3 diffuse = kd * max(dot(N, L), 0.0) * texColor.rgb;
    vec3 specular = ks * pow(max(dot(N, H), 0.0), shininess) * lightColor;

    vec3 result = ambient + diffuse + specular;
    

    fragColor = vec4(result, 1.0);
}