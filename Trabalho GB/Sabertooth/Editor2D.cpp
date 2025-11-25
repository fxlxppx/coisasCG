#include "Editor2D.h"

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>

// resolução da curva (menor = mais pontos)
static const float RESOLUTION = 0.01f;

// distância mínima entre pontos subsequentes para evitar pontos duplicados
static const float MIN_POINT_DIST = 0.015f;

// distância usada para fechar automaticamente (aumentada conforme pedido)
static float AUTO_CLOSE_DIST = 0.12f; // <-- aumentei para 0.12; ajuste aqui se quiser

static glm::vec2 catmull(const glm::vec2& p0,
    const glm::vec2& p1,
    const glm::vec2& p2,
    const glm::vec2& p3,
    float t)
{
    // Catmull-Rom centrada (tension 0.5) implementada de forma explícita:
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
    std::cout << "[Editor] ponto: " << p.x << " , " << p.y << "\n";
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
        std::cout << "[Editor] Fechamento automático ativado!\n";
        // REMOVER o último ponto (em vez de duplicar o primeiro), assim a topologia fica limpa.
        points.pop_back();
        closeCurve();
    }
}

void Editor2D::closeCurve()
{
    if (closed) return;
    if (points.size() < 4) {
        std::cout << "[ERRO] precisa de >=4 pontos.\n";
        return;
    }

    closed = true;
    splineCenter.clear();
    splineInner.clear();
    splineOuter.clear();

    int n = (int)points.size();

    // Gerar Catmull-Rom fechada (índices mod n)
    for (int i = 0; i < n; i++)
    {
        glm::vec2 p0 = points[(i - 1 + n) % n];
        glm::vec2 p1 = points[i];
        glm::vec2 p2 = points[(i + 1) % n];
        glm::vec2 p3 = points[(i + 2) % n];

        for (float t = 0.0f; t < 1.0f; t += RESOLUTION)
            splineCenter.push_back(catmull(p0, p1, p2, p3, t));
    }

    // Agora calcular normais de forma circular usando prev/next
    int m = (int)splineCenter.size();
    if (m < 2) return;

    for (int i = 0; i < m; i++)
    {
        int prevIdx = (i - 1 + m) % m;
        int nextIdx = (i + 1) % m;

        glm::vec2 prev = splineCenter[prevIdx];
        glm::vec2 p = splineCenter[i];
        glm::vec2 next = splineCenter[nextIdx];

        glm::vec2 tan = next - prev;
        float len = glm::length(tan);
        if (len < 1e-6f) tan = glm::vec2(1.0f, 0.0f);
        else tan /= len;

        glm::vec2 normal(-tan.y, tan.x);

        splineInner.push_back(p - width * normal);
        splineOuter.push_back(p + width * normal);
    }
}

void Editor2D::clear()
{
    points.clear();
    splineCenter.clear();
    splineInner.clear();
    splineOuter.clear();
    closed = false;
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

        float x = (float)(mx / (float)w * 2.0f - 1.0f);
        float y = (float)(1.0 - my / (float)h * 2.0f);

        addPoint(x, y);

        // tentar fechar automaticamente depois de adicionar
        tryAutoClose();
    }
    prevPressed = pressed;

    // teclas R = reset, C = fechar manual
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS)
        clear();

    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
        closeCurve();
}

void Editor2D::render()
{
    glDisable(GL_DEPTH_TEST);
    glUseProgram(0);

    // pontos
    glPointSize(10.0f);
    glColor3f(0.0f, 0.85f, 1.0f); // cor dos pontos (mais visível)
    glBegin(GL_POINTS);
    for (auto& p : points) glVertex2f(p.x, p.y);
    glEnd();

    // linha base (poligonal)
    if (points.size() >= 2)
    {
        glColor3f(0.85f, 0.85f, 0.85f);
        glBegin(closed ? GL_LINE_LOOP : GL_LINE_STRIP);
        for (auto& p : points) glVertex2f(p.x, p.y);
        glEnd();
    }

    if (!closed) return;

    // Catmull-Rom center
    glColor3f(1.0f, 1.0f, 0.0f);
    glBegin(GL_LINE_LOOP); // loop fechada para evitar gap
    for (auto& p : splineCenter) glVertex2f(p.x, p.y);
    glEnd();

    // Curva interna
    glColor3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    for (auto& p : splineInner) glVertex2f(p.x, p.y);
    glEnd();

    // Curva externa
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_LINE_LOOP);
    for (auto& p : splineOuter) glVertex2f(p.x, p.y);
    glEnd();
}