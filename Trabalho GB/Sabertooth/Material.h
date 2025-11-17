#pragma once
#include <string>
#include <glm/glm.hpp>
#include <GL/glew.h>


struct Material {
    std::string name;

    // Já existiam:
    glm::vec3 kd = glm::vec3(1.0f); // cor difusa
    unsigned int textureID = 0;
    bool hasTexture = false;

    // ADICIONAR:
    glm::vec3 ka = glm::vec3(0.1f);   // ambiente
    glm::vec3 ks = glm::vec3(1.0f);   // especular
    float shininess = 32.0f;          // Ns do MTL
};
