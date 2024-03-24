#pragma once

#include <glm/glm.hpp>

class Light {
public:
    glm::vec3 position;
    glm::vec3 color;

    Light(const glm::vec3& pos, const glm::vec3& col) : position(pos), color(col) {}
};