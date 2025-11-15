#include "MaterialLoader.h"
#include <fstream>
#include <sstream>
#include <iostream>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image_aug.h"
#include <GL/glew.h>

std::map<std::string, Material*> loadMTL(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir MTL: " << path << std::endl;
        return {};
    }

    std::map<std::string, Material*> materials;
    Material* current = nullptr;

    std::string line;
    while (std::getline(file, line))
    {
        std::stringstream ss(line);
        std::string type;
        ss >> type;

        if (type == "newmtl") {
            std::string name;
            ss >> name;
            current = new Material();
            current->name = name;
            materials[name] = current;
        }
        else if (type == "Kd" && current) {
            ss >> current->kd.r >> current->kd.g >> current->kd.b;
        }
        else if (type == "map_Kd" && current) {
            std::string texPath;
            ss >> texPath;

            int w, h, channels;
            unsigned char* data = stbi_load(texPath.c_str(), &w, &h, &channels, 0);
            if (data) {
                glGenTextures(1, &current->textureID);
                glBindTexture(GL_TEXTURE_2D, current->textureID);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,
                    channels == 4 ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                stbi_image_free(data);
                current->hasTexture = true;
                std::cout << "Textura carregada: " << texPath << std::endl;
            }
            else {
                std::cerr << "Falha ao carregar textura: " << texPath << std::endl;
            }
        }
    }

    std::cout << "MTL carregado: " << materials.size() << " materiais.\n";
    return materials;
}