#define _CRT_SECURE_NO_WARNINGS
#include "OBJLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Material.h"

Obj3D* loadOBJ(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir OBJ: " << path << std::endl;
        return nullptr;
    }

    Mesh* mesh = new Mesh();
    Group* currentGroup = new Group();
    currentGroup->name = "default";
    mesh->groups.push_back(currentGroup);

    std::string line;
    std::string mtlPath;

    std::vector<glm::vec3> tmpV;
    std::vector<glm::vec2> tmpVT;
    std::vector<glm::vec3> tmpVN;

    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "mtllib") {
            ss >> mtlPath;
        }
        else if (type == "usemtl") {
            std::string matName;
            ss >> matName;
            Material* m = new Material();
            m->name = matName;
            currentGroup->material = m;
        }
        else if (type == "g") {
            std::string groupName;
            ss >> groupName;

            currentGroup = new Group();
            currentGroup->name = groupName;
            mesh->groups.push_back(currentGroup);
        }
        else if (type == "v") {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            tmpV.push_back(v);
        }
        else if (type == "vt") {
            glm::vec2 vt;
            ss >> vt.x >> vt.y;
            tmpVT.push_back(vt);
        }
        else if (type == "vn") {
            glm::vec3 vn;
            ss >> vn.x >> vn.y >> vn.z;
            tmpVN.push_back(vn);
        }
        else if (type == "f") {
            std::vector<std::string> tokens;
            std::string tok;
            while (ss >> tok) tokens.push_back(tok);

            // Triângulo
            if (tokens.size() == 3) {
                Face* f = new Face();
                for (int i = 0; i < 3; i++) {
                    std::string t = tokens[i];
                    int v = -1, vt = -1, vn = -1;

                    // Tenta todos os formatos
                    if (sscanf(t.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {}
                    else if (sscanf(t.c_str(), "%d//%d", &v, &vn) == 2) {}
                    else if (sscanf(t.c_str(), "%d/%d", &v, &vt) == 2) {}
                    else sscanf(t.c_str(), "%d", &v);

                    f->v.push_back(v - 1);
                }
                currentGroup->faces.push_back(f);
            }
            // Quadrilátero (divide em dois triângulos)
            else if (tokens.size() == 4) {
                int idx[4];
                for (int i = 0; i < 4; i++) {
                    std::string t = tokens[i];
                    int v = -1, vt = -1, vn = -1;

                    if (sscanf(t.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {}
                    else if (sscanf(t.c_str(), "%d//%d", &v, &vn) == 2) {}
                    else if (sscanf(t.c_str(), "%d/%d", &v, &vt) == 2) {}
                    else sscanf(t.c_str(), "%d", &v);

                    idx[i] = v - 1;
                }

                Face* f1 = new Face();
                f1->v = { idx[0], idx[1], idx[2] };
                currentGroup->faces.push_back(f1);

                Face* f2 = new Face();
                f2->v = { idx[0], idx[2], idx[3] };
                currentGroup->faces.push_back(f2);
            }
            // Polígonos maiores (fan triangulation)
            else if (tokens.size() > 4) {
                int firstIndex = -1;
                std::vector<int> polyIndices;

                for (auto& t : tokens) {
                    int v = -1, vt = -1, vn = -1;
                    if (sscanf(t.c_str(), "%d/%d/%d", &v, &vt, &vn) == 3) {}
                    else if (sscanf(t.c_str(), "%d//%d", &v, &vn) == 2) {}
                    else if (sscanf(t.c_str(), "%d/%d", &v, &vt) == 2) {}
                    else sscanf(t.c_str(), "%d", &v);

                    polyIndices.push_back(v - 1);
                }

                firstIndex = polyIndices[0];
                for (size_t i = 1; i + 1 < polyIndices.size(); i++) {
                    Face* f = new Face();
                    f->v = { firstIndex, polyIndices[i], polyIndices[i + 1] };
                    currentGroup->faces.push_back(f);
                }
            }
        }
    }

    mesh->vertices = tmpV;

    // Debug
    size_t totalFaces = 0;
    for (auto g : mesh->groups) totalFaces += g->faces.size();

    std::cout << "---- OBJ Carregado ----" << std::endl;
    std::cout << "Verts: " << mesh->vertices.size() << std::endl;
    std::cout << "Faces: " << totalFaces << std::endl;

    Obj3D* obj = new Obj3D();
    obj->mesh = mesh;

    obj->mesh->uploadToGPU();
    return obj;
}