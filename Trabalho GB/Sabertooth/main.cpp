#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneLoader.h"
#include "Renderer.h"
#include "Camera.h"

Camera camera(glm::vec3(5.0f, 3.0f, 7.0f));
float deltaTime = 0.0f;
float lastFrame = 0.0f;

bool mouseCaptured = true;


static const int MAX_LIGHTS = 8;
bool globalLightEnabled = true;
bool lightEnabled[MAX_LIGHTS] = { true,true,true,true,true,true,true,true };


void toggleMouse(GLFWwindow* window)
{
    mouseCaptured = !mouseCaptured;

    if (mouseCaptured) {
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
    if (mouseCaptured)
        camera.processMouseMovement((float)xpos, (float)ypos);
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_W, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_S, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_A, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.processKeyboard(GLFW_KEY_D, deltaTime);

    
    static bool escPressed = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        if (!escPressed) {
            toggleMouse(window);
            escPressed = true;
        }
    }
    else escPressed = false;

    
    static bool Lpressed = false;
    if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
    {
        if (!Lpressed) {
            globalLightEnabled = !globalLightEnabled;
            Lpressed = true;
        }
    }
    else Lpressed = false;

    
    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        int key = GLFW_KEY_1 + i;
        static bool numPressed[MAX_LIGHTS] = {};
        if (glfwGetKey(window, key) == GLFW_PRESS)
        {
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

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(f, 1, &fSrc, nullptr);
    glCompileShader(f);

    GLuint program = glCreateProgram();
    glAttachShader(program, v);
    glAttachShader(program, f);
    glLinkProgram(program);

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

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    mouseCaptured = true;

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Erro ao iniciar GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.6f, 0.9f, 1.0f);

    GLuint shader = loadShader("Shaders/Core/core.vert", "Shaders/Core/core.frag");
    glUseProgram(shader);

    Scene* scene = loadScene("scene.txt");
    if (!scene) {
        std::cerr << "Falha ao carregar cena\n";
        return -1;
    }

    
    glm::mat4 proj = glm::perspective(glm::radians(60.0f),
        800.0f / 600.0f, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shader, "proj"),
        1, GL_FALSE, glm::value_ptr(proj));

   
    while (!glfwWindowShouldClose(window))
    {
        float time = (float)glfwGetTime();
        deltaTime = time - lastFrame;
        lastFrame = time;

        processInput(window);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shader);

       
        glm::mat4 view = camera.getViewMatrix();
        glUniformMatrix4fv(glGetUniformLocation(shader, "view"),
            1, GL_FALSE, glm::value_ptr(view));

        glUniform3fv(glGetUniformLocation(shader, "cameraPos"),
            1, glm::value_ptr(camera.position));

       
        int count = std::min((int)scene->lights.size(), MAX_LIGHTS);
        glUniform1i(glGetUniformLocation(shader, "lightCount"), count);

        glUniform1i(glGetUniformLocation(shader, "globalLightEnabled"),
            globalLightEnabled ? 1 : 0);

        for (int i = 0; i < count; i++)
        {
            std::string base = "lights[" + std::to_string(i) + "]";

            glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()),
                1, glm::value_ptr(scene->lights[i].position));
            glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()),
                1, glm::value_ptr(scene->lights[i].color));

            glUniform1i(glGetUniformLocation(shader, ("lightEnabled[" + std::to_string(i) + "]").c_str()),
                lightEnabled[i] ? 1 : 0);
        }

        
        for (Obj3D* obj : scene->objects)
        {
            obj->transform = glm::rotate(obj->transform,
                deltaTime * 0.6f,
                glm::vec3(0, 1, 0));

            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1,
                GL_FALSE, glm::value_ptr(obj->transform));

            Material* mat = obj->mesh->groups[0]->material;

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

            drawObject(obj, shader);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}