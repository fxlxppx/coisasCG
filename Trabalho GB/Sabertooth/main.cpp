#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneLoader.h"
#include "Renderer.h"
#include "Camera.h"
#include "Editor2D.h"

enum AppMode {
    MODE_EDITOR_2D = 0,
    MODE_3D = 1
};

AppMode mode = MODE_EDITOR_2D;

Camera camera(glm::vec3(5.0f, 3.0f, 7.0f));

float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool mouseCaptured = false;

static const int MAX_LIGHTS = 8;
bool globalLightEnabled = true;
bool lightEnabled[MAX_LIGHTS] = { true,true,true,true,true,true,true,true };

Scene* scene = nullptr;
GLuint shader = 0;

Editor2D editor;

void setMouseCaptured(GLFWwindow* window, bool state)
{
    mouseCaptured = state;
    if (state) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        camera.firstMouse = true;
        std::cout << "[Mouse] Capturado\n";
    }
    else {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        std::cout << "[Mouse] Liberado\n";
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    if (mode == MODE_3D && mouseCaptured)
        camera.processMouseMovement((float)xpos, (float)ypos);
}

void processInput(GLFWwindow* window)
{
    if (mode == MODE_EDITOR_2D)
    {
        static bool enterPressed = false;
        if (glfwGetKey(window, GLFW_KEY_ENTER) == GLFW_PRESS) {
            if (!enterPressed) {
                std::cout << "Mudando para modo 3D...\n";

                if (editor.closed) {
                    std::cout << "[Main] Exportando pista.obj...\n";
                    if (!editor.exportOBJ("pista.obj"))
                        std::cout << "[Main] exportOBJ retornou false.\n";
                }
                else {
                    std::cout << "[Main] Atenção: curva não está fechada, exportOBJ ignorado.\n";
                }

                mode = MODE_3D;
                setMouseCaptured(window, true);
                glClearColor(0.3f, 0.6f, 0.9f, 1.0f);

                camera.position = glm::vec3(6.0f, 4.0f, 8.0f);
                camera.yaw = -135.0f;
                camera.pitch = -20.0f;
                camera.updateCameraVectors();

                if (!scene) {
                    scene = loadScene("scene.txt");
                    if (!scene) {
                        std::cerr << "Falha ao carregar cena\n";
                        exit(1);
                    }
                }

                glUseProgram(shader);
            }
            enterPressed = true;
        }
        else enterPressed = false;

        return;
    }

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_W, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_S, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_A, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_D, deltaTime);

    float vSpeed = 10.0f;

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
        camera.position.y += vSpeed * deltaTime;

    if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
        camera.position.y -= vSpeed * deltaTime;

    static bool escPressed = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
        if (!escPressed) {
            setMouseCaptured(window, !mouseCaptured);
            escPressed = true;
        }
    }
    else escPressed = false;

    static bool Lpressed = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) {
        if (!Lpressed) {
            globalLightEnabled = !globalLightEnabled;
            Lpressed = true;
        }
    }
    else Lpressed = false;

    for (int i = 0; i < MAX_LIGHTS; i++) {
        int key = GLFW_KEY_1 + i;
        static bool numPressed[MAX_LIGHTS] = {};
        if (glfwGetKey(window, key) == GLFW_PRESS) {
            if (!numPressed[i]) {
                lightEnabled[i] = !lightEnabled[i];
                numPressed[i] = true;
            }
        }
        else numPressed[i] = false;
    }
}

GLuint loadShader(const char* vertPath, const char* fragPath)
{
    auto loadSrc = [](const char* p) {
        std::ifstream f(p);
        if (!f.is_open()) {
            std::cerr << "ERRO: Não foi possível abrir shader: " << p << "\n";
            return std::string();
        }
        std::stringstream ss; ss << f.rdbuf();
        return ss.str();
        };

    std::string vCode = loadSrc(vertPath);
    std::string fCode = loadSrc(fragPath);

    const char* vSrc = vCode.c_str();
    const char* fSrc = fCode.c_str();

    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(v, 1, &vSrc, nullptr);
    glCompileShader(v);

    GLint success = 0;
    glGetShaderiv(v, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024]; glGetShaderInfoLog(v, 1024, nullptr, log);
        std::cerr << "Vertex shader compile error:\n" << log << std::endl;
    }

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fSrc, nullptr);
    glCompileShader(f);
    glGetShaderiv(f, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[1024]; glGetShaderInfoLog(f, 1024, nullptr, log);
        std::cerr << "Fragment shader compile error:\n" << log << std::endl;
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glLinkProgram(program);
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        char log[1024]; glGetProgramInfoLog(program, 1024, nullptr, log);
        std::cerr << "Shader link error:\n" << log << std::endl;
    }

    glDeleteShader(v);
    glDeleteShader(f);

    return program;
}

int main()
{
    if (!glfwInit()) {
        std::cerr << "Erro ao iniciar GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(800, 600, "Trabalho Grau B", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erro ao criar janela\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);

    setMouseCaptured(window, false);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Erro ao iniciar GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);

    shader = loadShader("Shaders/Core/core.vert", "Shaders/Core/core.frag");
    if (shader == 0) {
        std::cerr << "Erro: shader invalido\n";
        return -1;
    }

    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    glm::mat4 proj = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);

    while (!glfwWindowShouldClose(window))
    {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;

        processInput(window);

        if (mode == MODE_EDITOR_2D)
        {
            glDisable(GL_DEPTH_TEST);
            glUseProgram(0);

            editor.update(window);
            editor.render();

            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        glEnable(GL_DEPTH_TEST);
        glUseProgram(shader);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shader, "proj"), 1, GL_FALSE, glm::value_ptr(proj));
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shader, "cameraPos"), 1, glm::value_ptr(camera.position));

        if (!scene) {
            glfwSwapBuffers(window);
            glfwPollEvents();
            continue;
        }

        int count = std::min((int)scene->lights.size(), MAX_LIGHTS);
        glUniform1i(glGetUniformLocation(shader, "lightCount"), count);
        glUniform1i(glGetUniformLocation(shader, "globalLightEnabled"), globalLightEnabled ? 1 : 0);

        for (int i = 0; i < count; i++) {
            std::string base = "lights[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()), 1, glm::value_ptr(scene->lights[i].position));
            glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()), 1, glm::value_ptr(scene->lights[i].color));
            glUniform1i(glGetUniformLocation(shader, ("lightEnabled[" + std::to_string(i) + "]").c_str()), lightEnabled[i] ? 1 : 0);
        }


        for (Obj3D* obj : scene->objects)
        {
            if (!obj || !obj->mesh || obj->mesh->groups.empty())
                continue;


            glUniformMatrix4fv(
                glGetUniformLocation(shader, "model"),
                1,
                GL_FALSE,
                glm::value_ptr(obj->transform)
            );

            Material defaultMat;
            defaultMat.ka = glm::vec3(0.2f);
            defaultMat.kd = glm::vec3(0.7f);
            defaultMat.ks = glm::vec3(0.1f);
            defaultMat.shininess = 16.0f;
            defaultMat.hasTexture = false;

            for (Group* g : obj->mesh->groups)
            {
                Material* mat = g->material ? g->material : &defaultMat;

                glUniform3fv(glGetUniformLocation(shader, "material.ka"), 1, glm::value_ptr(mat->ka));
                glUniform3fv(glGetUniformLocation(shader, "material.kd"), 1, glm::value_ptr(mat->kd));
                glUniform3fv(glGetUniformLocation(shader, "material.ks"), 1, glm::value_ptr(mat->ks));
                glUniform1f(glGetUniformLocation(shader, "material.shininess"), mat->shininess);
                glUniform1i(glGetUniformLocation(shader, "material.hasTexture"), mat->hasTexture);

                if (mat->hasTexture)
                {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mat->textureID);
                    glUniform1i(glGetUniformLocation(shader, "texSampler"), 0);
                }

                glBindVertexArray(g->VAO);
                glDrawArrays(GL_TRIANGLES, 0, g->numVertices);
            }
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}