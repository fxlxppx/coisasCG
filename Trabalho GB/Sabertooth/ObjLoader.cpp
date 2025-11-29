#define _CRT_SECURE_NO_WARNINGS
#include "OBJLoader.h"
#include "MaterialLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include "Material.h"
#include <vector>
#include <algorithm>
#include <map>

static void parseFaceToken(const std::string& token, int& v, int& vt, int& vn)
{
    v = vt = vn = -1;
    int slashCount = std::count(token.begin(), token.end(), '/');
    if (slashCount == 0)
    {
        int r = sscanf(token.c_str(), "%d", &v);
    }
    else if (slashCount == 1)
    {
        int r = sscanf(token.c_str(), "%d/%d", &v, &vt);
    }
    else
    {
        if (token.find("//") != std::string::npos)
        {
            int r = sscanf(token.c_str(), "%d//%d", &v, &vn);
        }
        else
        {
            int r = sscanf(token.c_str(), "%d/%d/%d", &v, &vt, &vn);
        }
    }
    
    if (v > 0) v--; else v = 0;
    if (vt > 0) vt--; else vt = -1;
    if (vn > 0) vn--; else vn = -1;
}

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

    std::vector<glm::vec3> tmpV;
    
    std::map<std::string, Material*> materials;
    Material* currentMaterial = nullptr;
    
    size_t lastSlash = path.find_last_of("/\\");
    std::string dir = (lastSlash != std::string::npos) ? path.substr(0, lastSlash + 1) : "";

    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "mtllib")
        {
            std::string mtlFile;
            ss >> mtlFile;
            std::string mtlPath = dir + mtlFile;
            
            std::cout << "[OBJ] Carregando MTL: " << mtlPath << std::endl;
            materials = loadMTL(mtlPath);
            
            if (materials.empty()) {
                std::cerr << "[OBJ] AVISO: Nenhum material carregado de " << mtlPath << std::endl;
            }
        }
        else if (type == "usemtl")
        {
            std::string matName;
            ss >> matName;
            
            if (materials.find(matName) != materials.end())
            {
                currentMaterial = materials[matName];
                
                currentGroup = new Group();
                currentGroup->name = matName;
                currentGroup->material = currentMaterial;
                mesh->groups.push_back(currentGroup);
                
                std::cout << "[OBJ] Usando material: " << matName << std::endl;
            }
            else
            {
                std::cerr << "[OBJ] AVISO: Material '" << matName << "' não encontrado!" << std::endl;
            }
        }
        else if (type == "v")
        {
            glm::vec3 v;
            ss >> v.x >> v.y >> v.z;
            tmpV.push_back(v);
        }
        else if (type == "f")
        {
            std::vector<std::string> tokens;
            std::string t;
            while (ss >> t)
                tokens.push_back(t);

            if (tokens.size() < 3) continue;

            std::vector<int> verts;
            verts.reserve(tokens.size());
            for (auto& tok : tokens)
            {
                int v, vt, vn;
                parseFaceToken(tok, v, vt, vn);
                verts.push_back(v);
            }

            // Triangulação em fan
            for (size_t i = 1; i + 1 < verts.size(); i++)
            {
                Face* f = new Face();
                f->v = { verts[0], verts[i], verts[i + 1] };
                currentGroup->faces.push_back(f);
            }
        }
    }

    mesh->vertices = tmpV;

    size_t totalFaces = 0;
    for (auto g : mesh->groups) totalFaces += g->faces.size();

    std::cout << "---- OBJ Carregado ----\n";
    std::cout << "Arquivo: " << path << "\n";
    std::cout << "Vértices: " << mesh->vertices.size() << "\n";
    std::cout << "Faces: " << totalFaces << "\n";
    std::cout << "Grupos/Materiais: " << mesh->groups.size() << "\n";
    std::cout << "-----------------------\n";

    Obj3D* obj = new Obj3D();
    obj->mesh = mesh;
    obj->mesh->uploadToGPU();

    return obj;
}