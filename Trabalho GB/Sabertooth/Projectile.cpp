#include "Projectile.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

void ProjectileManager::spawn(const glm::vec3& position, const glm::vec3& direction, float currentTime)
{
    Projectile p;
    p.pos = position;
    p.dir = glm::normalize(direction);
    p.spawnTime = currentTime;
    p.model = glm::translate(glm::mat4(1.0f), p.pos) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));

    projectiles.push_back(p);

    std::cout << "[Proj] Disparado. pos=(" << p.pos.x << "," << p.pos.y << "," << p.pos.z
        << ") dir=(" << p.dir.x << "," << p.dir.y << "," << p.dir.z << ")\n";
}

void ProjectileManager::update(float deltaTime, float currentTime)
{
    for (size_t i = 0; i < projectiles.size();) {
        Projectile& p = projectiles[i];
        float life = currentTime - p.spawnTime;

        if (life > 2.0f) {
            projectiles.erase(projectiles.begin() + i);
            continue;
        }

        p.pos += p.dir * projectileSpeed * deltaTime;
        p.model = glm::translate(glm::mat4(1.0f), p.pos) *
            glm::scale(glm::mat4(1.0f), glm::vec3(0.3f, 0.3f, 0.3f));

        ++i;
    }
}