#include "SceneLoader.h"
#include "OBJLoader.h"

#include <fstream>
#include <sstream>
#include <iostream>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

Scene* loadScene(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir cena: " << path << std::endl;
        return nullptr;
    }

    Scene* scene = new Scene();

    std::string line;
    while (std::getline(file, line))
    {
        if (line.empty() || line[0] == '#')
            continue;

        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "model")
        {
            std::string modelPath;
            float px, py, pz;
            float sx, sy, sz;
            float rx, ry, rz;

            ss >> modelPath >> px >> py >> pz >> sx >> sy >> sz >> rx >> ry >> rz;

            Obj3D* obj = loadOBJ(modelPath);
            if (!obj) {
                std::cerr << "Falha ao carregar modelo: " << modelPath << std::endl;
                continue;
            }

            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, glm::vec3(px, py, pz));
            transform = glm::rotate(transform, glm::radians(rx), glm::vec3(1, 0, 0));
            transform = glm::rotate(transform, glm::radians(ry), glm::vec3(0, 1, 0));
            transform = glm::rotate(transform, glm::radians(rz), glm::vec3(0, 0, 1));
            transform = glm::scale(transform, glm::vec3(sx, sy, sz));

            obj->transform = transform;
            scene->objects.push_back(obj);
        }

        else if (type == "obj")
        {
            std::string modelPath;
            ss >> modelPath;

            Obj3D* obj = loadOBJ(modelPath);
            if (!obj) {
                std::cerr << "Falha ao carregar OBJ: " << modelPath << std::endl;
                continue;
            }

            obj->transform = glm::mat4(1.0f); 
            scene->objects.push_back(obj);
        }

        else if (type == "track")
        {
            std::string modelPath;
            float px, py, pz;
            float scale;
            float rx, ry, rz;

            ss >> modelPath >> px >> py >> pz >> scale >> rx >> ry >> rz;

            Obj3D* obj = loadOBJ(modelPath);
            if (!obj) {
                std::cerr << "Falha ao carregar track: " << modelPath << std::endl;
                continue;
            }

            glm::mat4 transform = glm::mat4(1.0f);
            transform = glm::translate(transform, glm::vec3(px, py, pz));
            transform = glm::rotate(transform, glm::radians(rx), glm::vec3(1, 0, 0));
            transform = glm::rotate(transform, glm::radians(ry), glm::vec3(0, 1, 0));
            transform = glm::rotate(transform, glm::radians(rz), glm::vec3(0, 0, 1));
            transform = glm::scale(transform, glm::vec3(scale));

            obj->transform = transform;
            scene->objects.push_back(obj);
        }

        else if (type == "light")
        {
            float px, py, pz;
            float r, g, b;

            ss >> px >> py >> pz >> r >> g >> b;

            Light L;
            L.position = glm::vec3(px, py, pz);
            L.color = glm::vec3(r, g, b);

            scene->lights.push_back(L);
        }
    }

    std::cout << "Cena carregada com "
        << scene->objects.size() << " objetos e "
        << scene->lights.size() << " luz(es).\n";

    return scene;
}