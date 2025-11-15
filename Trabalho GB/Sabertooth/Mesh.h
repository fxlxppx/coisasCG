#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "Group.h"

class Mesh {
public:
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texcoords;

    std::vector<Group*> groups;

    void uploadToGPU();
};