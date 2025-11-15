#include "Mesh.h"
#include <GL/glew.h>

void Mesh::uploadToGPU() {

    for (Group* g : groups)
    {
        std::vector<float> bufferPositions;
        bufferPositions.reserve(g->faces.size() * 9); // 3 vertices * (x,y,z)

        for (Face* f : g->faces)
        {
            for (int idx : f->v)
            {
                glm::vec3 p = vertices[idx];
                bufferPositions.push_back(p.x);
                bufferPositions.push_back(p.y);
                bufferPositions.push_back(p.z);
            }
        }

        g->numVertices = bufferPositions.size() / 3;

        // VAO
        glGenVertexArrays(1, &g->VAO);
        glBindVertexArray(g->VAO);

        // VBO armazenado no Group
        glGenBuffers(1, &g->VBO);
        glBindBuffer(GL_ARRAY_BUFFER, g->VBO);
        glBufferData(GL_ARRAY_BUFFER, bufferPositions.size() * sizeof(float), bufferPositions.data(), GL_STATIC_DRAW);

        // Layout do atributo (posição)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

        // IMPORTANTE: fechar o VAO!
        glBindVertexArray(0);
    }
}