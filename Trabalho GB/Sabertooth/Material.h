#pragma once
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>

class Material {
public:
    std::string name;
    glm::vec3 kd = glm::vec3(1.0f, 0.5f, 0.2f); // cor difusa padrão (laranja)
    GLuint textureID = 0;
    bool hasTexture = false;

    Material() = default;
};