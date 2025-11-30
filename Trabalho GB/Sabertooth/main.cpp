#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "SceneLoader.h"
#include "Renderer.h"
#include "Camera.h"
#include "Editor2D.h"
#include "ObjLoader.h"
#include "Projectile.h"

enum AppMode { MODE_EDITOR_2D = 0, MODE_3D = 1 };
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

float trackScale = 20.0f;
float TRACK_HEIGHT = 0.05f;
float carHeightOffset = 0.5f;
float carSpeed = 3.0f;

std::vector<glm::vec3> carPath;
std::vector<float> carAccumLen;
float carTotalLength = 0.0f;
float carTravelS = 0.0f;

Obj3D* carObj = nullptr;
Obj3D* projectileObj = nullptr;
glm::vec3 carOriginalScale(1.0f);
float carMinYLocal = 0.0f;

glm::mat4 proj;

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

ProjectileManager projectileManager;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (mode != MODE_3D) return;
    if (button != GLFW_MOUSE_BUTTON_LEFT) return;
    if (action != GLFW_PRESS) return;

    double mx, my;
    glfwGetCursorPos(window, &mx, &my);

    glm::vec3 startPos = camera.position;
    projectileManager.spawn(startPos, camera.front, (float)glfwGetTime());

}

void buildCarPathFromEditor()
{
    carPath.clear();
    carAccumLen.clear();
    carTotalLength = 0.0f;
    carTravelS = 0.0f;

    if (editor.splineCenter.empty()) {
        std::cout << "[Car] splineCenter vazia — nada a construir.\n";
        return;
    }

    for (auto& p : editor.splineCenter) {
        glm::vec3 v;
        v.x = p.x * trackScale;
        v.y = TRACK_HEIGHT + carHeightOffset;
        v.z = -p.y * trackScale;
        carPath.push_back(v);
    }

    if (carPath.size() >= 2) {
        size_t n = carPath.size();
        carAccumLen.resize(n);
        carAccumLen[0] = 0.0f;

        for (size_t i = 1; i < n; ++i) {
            float d = glm::length(carPath[i] - carPath[i - 1]);
            carAccumLen[i] = carAccumLen[i - 1] + d;
        }

        float closing = glm::length(carPath[0] - carPath.back());
        carTotalLength = carAccumLen.back() + closing;
    }
    else {
        carTotalLength = 0.0f;
    }

    std::cout << "[Car] Path construído: pontos=" << carPath.size()
        << " comprimento total=" << carTotalLength << "\n";
}

void sampleCarPath(float s, glm::vec3& outPos, glm::vec3& outTangent)
{
    if (carPath.empty()) { outPos = glm::vec3(0); outTangent = glm::vec3(1, 0, 0); return; }
    if (carTotalLength <= 1e-6f) { outPos = carPath[0]; outTangent = glm::vec3(1, 0, 0); return; }

    while (s < 0.0f) s += carTotalLength;
    while (s >= carTotalLength) s -= carTotalLength;

    size_t n = carPath.size();

    if (s <= 0.0f) {
        outPos = carPath[0];
        outTangent = glm::normalize(carPath[1] - carPath[0]);
        return;
    }

    size_t idx = 0;
    for (size_t i = 0; i + 1 < n; i++) {
        if (s >= carAccumLen[i] && s < carAccumLen[i + 1]) { idx = i; break; }
        if (i + 1 == n - 1 && s >= carAccumLen[n - 1]) { idx = n - 1; break; }
    }

    if (idx < n - 1) {
        float segStart = carAccumLen[idx];
        float segEnd = carAccumLen[idx + 1];
        float segLen = segEnd - segStart;
        float local = (s - segStart) / segLen;

        outPos = glm::mix(carPath[idx], carPath[idx + 1], local);
        outTangent = glm::normalize(carPath[idx + 1] - carPath[idx]);
    }
    else {
        float segStart = carAccumLen[n - 1];
        float segLen = carTotalLength - segStart;
        float local = (s - segStart) / segLen;

        outPos = glm::mix(carPath[n - 1], carPath[0], local);
        outTangent = glm::normalize(carPath[0] - carPath[n - 1]);
    }
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
                    editor.exportOBJ("pista.obj");
                }

                mode = MODE_3D;
                setMouseCaptured(window, true);
                glClearColor(0.3f, 0.6f, 0.9f, 1.0f);

                camera.position = glm::vec3(6, 4, 8);
                camera.yaw = -135.0f;
                camera.pitch = -20.0f;
                camera.updateCameraVectors();

                if (!scene) {
                    scene = loadScene("scene.txt");
                    if (!scene) exit(1);
                }

                if (!scene->objects.empty()) {
                    carObj = scene->objects[0];
					projectileObj = loadOBJ("Cube.obj");

                    glm::mat4 t = carObj->transform;
                    carOriginalScale.x = glm::length(glm::vec3(t[0][0], t[1][0], t[2][0]));
                    carOriginalScale.y = glm::length(glm::vec3(t[0][1], t[1][1], t[2][1]));
                    carOriginalScale.z = glm::length(glm::vec3(t[0][2], t[1][2], t[2][2]));

                    if (carObj->mesh && !carObj->mesh->vertices.empty()) {
                        float minY = carObj->mesh->vertices[0].y;
                        for (auto& v : carObj->mesh->vertices)
                            if (v.y < minY) minY = v.y;

                        carMinYLocal = minY;
                    }
                }

                buildCarPathFromEditor();
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

    float vSpeed = 5.0f;
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
    auto loadSrc = [&](const char* p) {
        std::ifstream f(p);
        if (!f.is_open()) return std::string();
        std::stringstream ss; ss << f.rdbuf();
        return ss.str();
        };

    std::string vCode = loadSrc(vertPath);
    std::string fCode = loadSrc(fragPath);

    GLuint v = glCreateShader(GL_VERTEX_SHADER);
    const char* vSrc = vCode.c_str();
    glShaderSource(v, 1, &vSrc, nullptr);
    glCompileShader(v);

    GLint ok;
    glGetShaderiv(v, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(v, 1024, nullptr, log); std::cerr << log; }

    GLuint f = glCreateShader(GL_FRAGMENT_SHADER);
    const char* fSrc = fCode.c_str();
    glShaderSource(f, 1, &fSrc, nullptr);
    glCompileShader(f);

    glGetShaderiv(f, GL_COMPILE_STATUS, &ok);
    if (!ok) { char log[1024]; glGetShaderInfoLog(f, 1024, nullptr, log); std::cerr << log; }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);

    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) { char log[1024]; glGetProgramInfoLog(prog, 1024, nullptr, log); std::cerr << log; }

    glDeleteShader(v);
    glDeleteShader(f);

    return prog;
}

int main()
{
    if (!glfwInit()) return -1;

    GLFWwindow* window = glfwCreateWindow(800, 600, "Trabalho Grau B", nullptr, nullptr);
    if (!window) return -1;

    glfwMakeContextCurrent(window);
    glfwSetCursorPosCallback(window, mouse_callback);

    glfwSetMouseButtonCallback(window, mouse_button_callback);

    setMouseCaptured(window, false);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK) return -1;

    glEnable(GL_DEPTH_TEST);

    shader = loadShader("Shaders/Core/core.vert", "Shaders/Core/core.frag");
    if (shader == 0) return -1;

    proj = glm::perspective(glm::radians(60.0f), 800.0f / 600.0f, 0.1f, 100.0f);

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
        glUniform1i(glGetUniformLocation(shader, "globalLightEnabled"), globalLightEnabled);

        for (int i = 0; i < count; i++) {
            std::string base = "lights[" + std::to_string(i) + "]";
            glUniform3fv(glGetUniformLocation(shader, (base + ".position").c_str()), 1, glm::value_ptr(scene->lights[i].position));
            glUniform3fv(glGetUniformLocation(shader, (base + ".color").c_str()), 1, glm::value_ptr(scene->lights[i].color));
            glUniform1i(glGetUniformLocation(shader, ("lightEnabled[" + std::to_string(i) + "]").c_str()), lightEnabled[i] ? 1 : 0);
        }

        if (!carPath.empty() && carTotalLength > 0.001f && carObj != nullptr)
        {
            carTravelS += carSpeed * deltaTime;
            while (carTravelS >= carTotalLength) carTravelS -= carTotalLength;

            glm::vec3 pos, tan;
            sampleCarPath(carTravelS, pos, tan);

            glm::vec3 forward(0, 0, 1);
            float yaw = std::atan2(forward.z, forward.x) - std::atan2(tan.z, tan.x);

            float lift = (TRACK_HEIGHT + carHeightOffset) - (carMinYLocal * carOriginalScale.y);

            glm::mat4 model = glm::mat4(1);
            model = glm::translate(model, glm::vec3(pos.x, pos.y + lift, pos.z));
            model = glm::rotate(model, yaw, glm::vec3(0, 1, 0));
            model = glm::scale(model, carOriginalScale);

            carObj->transform = model;
        }

        for (Obj3D* obj : scene->objects)
        {
            if (!obj || !obj->mesh || obj->mesh->groups.empty())
                continue;

            glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(obj->transform));

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

		projectileManager.update(deltaTime, time);

        if (projectileObj && projectileObj->mesh) {
            for (Projectile& p : projectileManager.projectiles) {
                glUniformMatrix4fv(glGetUniformLocation(shader, "model"), 1, GL_FALSE, glm::value_ptr(p.model));

                Material defaultMat;
                defaultMat.ka = glm::vec3(0.2f);
                defaultMat.kd = glm::vec3(0.7f);
                defaultMat.ks = glm::vec3(0.1f);
                defaultMat.shininess = 16.0f;
                defaultMat.hasTexture = false;

                for (Group* g : projectileObj->mesh->groups)
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
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
