#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <GL/glew.h>
#include <GLFW/glfw3.h>

class Editor2D {
public:
    std::vector<glm::vec2> points;

    bool closed = false;
    float width = 0.06f;

    std::vector<glm::vec2> splineCenter;
    std::vector<glm::vec2> splineInner;
    std::vector<glm::vec2> splineOuter;

    void update(GLFWwindow* window);
    void render();

    void addPoint(double x, double y);
    void tryAutoClose();
    void closeCurve();
    void clear();

    bool exportOBJ(const char* filename);
};