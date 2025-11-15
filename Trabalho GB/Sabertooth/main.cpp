#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneLoader.h"
#include "Renderer.h"
#include "Camera.h"

// Globals
Camera camera(glm::vec3(5.0f, 3.0f, 7.0f));
float lastFrame = 0.0f;
float deltaTime = 0.0f;
int windowWidth = 800;
int windowHeight = 600;

// Mouse callback
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    camera.processMouseMovement((float)xpos, (float)ypos);
}

void mouse_click_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        camera.firstMouse = true; // evitar jump
    }
}

// simple shader loader (with error prints)
GLuint loadShader(const char* vertPath, const char* fragPath) {
    auto readFile = [](const char* path) -> std::string {
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "ERRO: Não foi possível abrir shader: " << path << std::endl;
            return std::string();
        }
        std::stringstream s;
        s << file.rdbuf();
        return s.str();
        };

    std::string vertCode = readFile(vertPath);
    std::string fragCode = readFile(fragPath);
    const char* vSrc = vertCode.c_str();
    const char* fSrc = fragCode.c_str();

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vSrc, NULL);
    glCompileShader(vertexShader);
    {
        GLint ok; glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[1024]; glGetShaderInfoLog(vertexShader, 1024, nullptr, buf);
            std::cerr << "Vertex shader compile error:\n" << buf << std::endl;
        }
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fSrc, NULL);
    glCompileShader(fragmentShader);
    {
        GLint ok; glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char buf[1024]; glGetShaderInfoLog(fragmentShader, 1024, nullptr, buf);
            std::cerr << "Fragment shader compile error:\n" << buf << std::endl;
        }
    }

    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    {
        GLint ok; glGetProgramiv(program, GL_LINK_STATUS, &ok);
        if (!ok) {
            char buf[1024]; glGetProgramInfoLog(program, 1024, nullptr, buf);
            std::cerr << "Shader link error:\n" << buf << std::endl;
        }
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}

int main()
{
    if (!glfwInit()) {
        std::cerr << "Erro ao iniciar GLFW\n";
        return -1;
    }

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Meu Projeto 3D", nullptr, nullptr);
    if (!window) {
        std::cerr << "Erro ao criar janela GLFW\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    // set callbacks and mouse mode
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_click_callback);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) {
        std::cerr << "Erro ao iniciar GLEW\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.3f, 0.6f, 0.9f, 1.0f);

    // load shader
    GLuint shaderProgram = loadShader("Shaders/Core/core.vert", "Shaders/Core/core.frag");
    glUseProgram(shaderProgram);

    // load scene
    Scene* scene = loadScene("scene.txt");
    if (!scene) {
        std::cerr << "Falha ao carregar cena\n";
        return -1;
    }

    // projection uniform (only once or update on resize)
    glm::mat4 proj = glm::perspective(glm::radians(60.0f), (float)windowWidth / (float)windowHeight, 0.1f, 100.0f);
    GLint locProj = glGetUniformLocation(shaderProgram, "proj");
    glUniformMatrix4fv(locProj, 1, GL_FALSE, glm::value_ptr(proj));

    // main loop
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // process keyboard (WASD)
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_W, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_S, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_A, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.processKeyboard(GLFW_KEY_D, deltaTime);
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glUseProgram(shaderProgram);

        // view
        glm::mat4 view = camera.getViewMatrix();
        GLint locView = glGetUniformLocation(shaderProgram, "view");
        glUniformMatrix4fv(locView, 1, GL_FALSE, glm::value_ptr(view));

        // draw objects
        for (Obj3D* obj : scene->objects)
        {
            // rotate a bit each frame (accumulativo)
            obj->transform = glm::rotate(obj->transform, deltaTime * 0.8f, glm::vec3(0, 1, 0));

            GLint locModel = glGetUniformLocation(shaderProgram, "model");
            glUniformMatrix4fv(locModel, 1, GL_FALSE, glm::value_ptr(obj->transform));

            drawObject(obj, shaderProgram);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
