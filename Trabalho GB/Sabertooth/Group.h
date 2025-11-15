#pragma once
#include <vector>
#include <string>
#include <GL/glew.h>

#include "Face.h"
#include "Material.h"

class Group {
public:
    std::string name;    
    Material* material = nullptr;

    std::vector<Face*> faces;

    GLuint VAO = 0;
    GLuint VBO = 0;
    int numVertices = 0;
};