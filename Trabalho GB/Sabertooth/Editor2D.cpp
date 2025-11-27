#include "Editor2D.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <cmath>

static float RESOLUTION = 0.09f;


static const float MIN_POINT_DIST = 0.015f;


static float AUTO_CLOSE_DIST = 0.10f;


static const float TRACK_HEIGHT = 0.05f;



static glm::vec2 catmull(const glm::vec2& p0,
    const glm::vec2& p1,
    const glm::vec2& p2,
    const glm::vec2& p3,
    float t)
{
    float t2 = t * t;
    float t3 = t2 * t;

    glm::vec2 a = 2.0f * p1;
    glm::vec2 b = p2 - p0;
    glm::vec2 c = 2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3;
    glm::vec2 d = -p0 + 3.0f * p1 - 3.0f * p2 + p3;

    return 0.5f * (a + b * t + c * t2 + d * t3);
}


void Editor2D::addPoint(double x, double y)
{
    if (closed) return;

    glm::vec2 p((float)x, (float)y);

    if (!points.empty()) {
        glm::vec2 last = points.back();
        float d2 = glm::dot(p - last, p - last);
        if (d2 < MIN_POINT_DIST * MIN_POINT_DIST)
            return;
    }

    points.push_back(p);
}


void Editor2D::tryAutoClose()
{
    if (closed) return;
    if (points.size() < 4) return;

    glm::vec2 first = points[0];
    glm::vec2 last = points.back();

    float d2 = glm::dot(last - first, last - first);

    if (d2 < AUTO_CLOSE_DIST * AUTO_CLOSE_DIST)
    {
        std::cout << "[Editor] Fechamento automático!\n";

        points.pop_back();

        closeCurve();
    }
}


void Editor2D::closeCurve()
{
    if (closed) return;
    if (points.size() < 4) {
        std::cout << "[ERRO] precisa de >= 4 pontos.\n";
        return;
    }

    closed = true;

    splineCenter.clear();
    splineInner.clear();
    splineOuter.clear();

    int n = points.size();


    for (int i = 0; i < n; i++)
    {
        glm::vec2 p0 = points[(i - 1 + n) % n];
        glm::vec2 p1 = points[i];
        glm::vec2 p2 = points[(i + 1) % n];
        glm::vec2 p3 = points[(i + 2) % n];

        for (float t = 0; t < 1.0f; t += RESOLUTION)
            splineCenter.push_back(catmull(p0, p1, p2, p3, t));
    }

    int m = splineCenter.size();

    {
        std::vector<glm::vec2> clean;
        clean.reserve(m);

        glm::vec2 last = splineCenter[0];
        clean.push_back(last);

        for (int i = 1; i < m; i++) {
            float d2 = glm::dot(splineCenter[i] - last,
                splineCenter[i] - last);

            if (d2 > 0.0001f) {  
                clean.push_back(splineCenter[i]);
                last = splineCenter[i];
            }
        }

        splineCenter = clean;
        m = splineCenter.size();
    }


    for (int i = 0; i < m; i++)
    {
        int prev = (i - 1 + m) % m;
        int next = (i + 1) % m;

        glm::vec2 tan = splineCenter[next] - splineCenter[prev];
        float len = glm::length(tan);

        if (len < 1e-6f) tan = glm::vec2(1, 0);
        else tan /= len;

        glm::vec2 normal(-tan.y, tan.x);

        splineInner.push_back(splineCenter[i] - normal * width);
        splineOuter.push_back(splineCenter[i] + normal * width);
    }
}



bool Editor2D::exportOBJ(const char* filename)
{
    if (!closed) {
        std::cout << "[ERRO] curva não está fechada.\n";
        return false;
    }

    int m = splineOuter.size();
    if (m == 0) {
        std::cout << "[ERRO] spline vazia.\n";
        return false;
    }

    std::ofstream out(filename);
    if (!out.is_open()) {
        std::cout << "[ERRO] não foi possível criar OBJ.\n";
        return false;
    }

    std::cout << "[OBJ] Gerando pista leve...\n";

    
    for (auto& p : splineOuter)
        out << "v " << p.x << " " << TRACK_HEIGHT << " " << -p.y << "\n";

    
    int offsetInner = splineOuter.size();

   
    for (auto& p : splineInner)
        out << "v " << p.x << " " << TRACK_HEIGHT << " " << -p.y << "\n";

    
    out << "g Track\n";

    for (int i = 0; i < m; i++)
    {
        int i2 = (i + 1) % m;

        int o1 = i + 1;
        int o2 = i2 + 1;

        int in1 = offsetInner + i + 1;
        int in2 = offsetInner + i2 + 1;

        out << "f " << o1 << " " << in1 << " " << o2 << "\n";
        out << "f " << in1 << " " << in2 << " " << o2 << "\n";
    }

    out.close();

    std::cout << "[OBJ] Sucesso! Vértices: " << (m * 2)
        << "  Faces: " << (m * 2) << "\n";

    return true;
}



void Editor2D::clear()
{
    closed = false;
    points.clear();
    splineCenter.clear();
    splineInner.clear();
    splineOuter.clear();
}



void Editor2D::update(GLFWwindow* window)
{
    static bool prevPressed = false;
    bool pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

    if (pressed && !prevPressed)
    {
        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        int w, h;
        glfwGetWindowSize(window, &w, &h);

        float x = (mx / w) * 2.0f - 1.0f;
        float y = 1.0f - (my / h) * 2.0f;

        addPoint(x, y);
        tryAutoClose();
    }
    prevPressed = pressed;

    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        clear();

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        closeCurve();
}



void Editor2D::render()
{
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0);

    
    glPointSize(10);
    glColor3f(0, 0.8f, 1);
    glBegin(GL_POINTS);
    for (auto& p : points) glVertex2f(p.x, p.y);
    glEnd();

    
    if (points.size() >= 2)
    {
        glColor3f(0.85f, 0.85f, 0.85f);
        glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
        for (auto& p : points) glVertex2f(p.x, p.y);
        glEnd();
    }

    if (!closed) return;

    
    glColor3f(1, 1, 0);
    glBegin(GL_LINE_LOOP);
    for (auto& p : splineCenter) glVertex2f(p.x, p.y);
    glEnd();

    
    glColor3f(0, 1, 0);
    glBegin(GL_LINE_LOOP);
    for (auto& p : splineInner) glVertex2f(p.x, p.y);
    glEnd();

    
    glColor3f(1, 0, 0);
    glBegin(GL_LINE_LOOP);
    for (auto& p : splineOuter) glVertex2f(p.x, p.y);
    glEnd();
}