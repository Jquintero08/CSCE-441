#pragma once
#pragma once

#include <glm/glm.hpp>

class Material {
public:
    glm::vec3 ka;
    glm::vec3 kd;
    glm::vec3 ks;
    float shininess;

    Material(const glm::vec3& a, const glm::vec3& d, const glm::vec3& s, float sh) :
        ka(a), kd(d), ks(s), shininess(sh) {}
};