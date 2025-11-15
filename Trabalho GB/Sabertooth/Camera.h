#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Camera
{
public:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;

    float speed;
    float mouseSensitivity;

    float lastX;
    float lastY;
    bool firstMouse;

    Camera(glm::vec3 startPos = glm::vec3(0.0f, 1.8f, 5.0f));

    glm::mat4 getViewMatrix();
    void processKeyboard(int key, float deltaTime);
    void processMouseMovement(float xpos, float ypos);
    void updateCameraVectors(); // deixo público (para compatibilidade)
};