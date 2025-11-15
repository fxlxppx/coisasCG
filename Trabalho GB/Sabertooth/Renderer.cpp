#include "Renderer.h"
#include "Group.h"
#include "Mesh.h"
#include "Face.h"
#include "Material.h"
#include <GL/glew.h>

void drawObject(Obj3D* obj, GLuint shaderProgram)
{
    for (Group* g : obj->mesh->groups)
    {
        if (g->material)
        {
            GLint hasTexLoc = glGetUniformLocation(shaderProgram, "hasTexture");
            GLint colorLoc = glGetUniformLocation(shaderProgram, "color");

            glUniform1i(hasTexLoc, g->material->hasTexture ? 1 : 0);
            glUniform3fv(colorLoc, 1, &g->material->kd[0]);

            if (g->material->hasTexture)
            {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, g->material->textureID);
                glUniform1i(glGetUniformLocation(shaderProgram, "tex"), 0);
            }
        }

        glBindVertexArray(g->VAO);
        glDrawArrays(GL_TRIANGLES, 0, g->numVertices);
    }
}