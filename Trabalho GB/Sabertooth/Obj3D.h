#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "Mesh.h"

class Obj3D {
public:
    Mesh* mesh = nullptr;
    glm::mat4 transform;

    Obj3D() {
        transform = glm::mat4(1.0f); // matriz identidade
    }
};