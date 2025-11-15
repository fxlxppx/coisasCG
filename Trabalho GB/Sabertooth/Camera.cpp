#include "Camera.h"
#include <GLFW/glfw3.h>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

Camera::Camera(glm::vec3 startPos)
{
    position = startPos;
    worldUp = glm::vec3(0.0f, 1.0f, 0.0f);

    yaw = -90.0f;
    pitch = 0.0f;

    speed = 5.0f;
    mouseSensitivity = 0.1f;

    front = glm::vec3(0.0f, 0.0f, -1.0f);
    firstMouse = true;
    lastX = 400.0f;
    lastY = 300.0f;

    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(position, position + front, up);
}

void Camera::processKeyboard(int key, float deltaTime)
{
    float velocity = speed * deltaTime;
    if (key == GLFW_KEY_W)
        position += front * velocity;
    if (key == GLFW_KEY_S)
        position -= front * velocity;
    if (key == GLFW_KEY_A)
        position -= right * velocity;
    if (key == GLFW_KEY_D)
        position += right * velocity;
}

void Camera::processMouseMovement(float xpos, float ypos)
{
    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // invertido
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    glm::vec3 f;
    f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    f.y = sin(glm::radians(pitch));
    f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));

    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}