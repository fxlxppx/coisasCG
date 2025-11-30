#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Projectile {
    glm::vec3 pos;
    glm::vec3 dir;
    float spawnTime;
    glm::mat4 model;
};

class ProjectileManager {
public:
    std::vector<Projectile> projectiles;
    float projectileSpeed = 40.0f;

    void spawn(const glm::vec3& position, const glm::vec3& direction, float currentTime);
    void update(float deltaTime, float currentTime);
    int getCount() const { return projectiles.size(); }
};