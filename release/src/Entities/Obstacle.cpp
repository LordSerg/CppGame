#include "Obstacle.h"
#include "../Graphics/Renderer.h"

Obstacle::Obstacle(int id, ObstacleType type, int x, int y)
    : Entity(id, EntityType::OBSTACLE, -1)
    , obstacleType(type)
{
    position = Vector2(x * 32.0f, y * 32.0f);
    
    if (type == ObstacleType::ROCK) {
        width = 2;
        height = 2;
        maxHealth = 100;
        currentHealth = 100;
    } else {
        width = 1;
        height = 1;
        maxHealth = 50;
        currentHealth = 50;
    }
}

void Obstacle::Update(float deltaTime) {
    // Obstacles don't update
}

void Obstacle::Render(Renderer* renderer) {
    // Render obstacle sprite
    Vector2 screenPos = renderer->WorldToScreen(position);
    
    glm::vec3 color(0.5f, 0.5f, 0.5f);
    if (obstacleType == ObstacleType::TREE) {
        color = glm::vec3(0.2f, 0.6f, 0.2f);
    }
    
    renderer->DrawRect(
        Rect(screenPos.x, screenPos.y, width * 32, height * 32),
        color
    );
}