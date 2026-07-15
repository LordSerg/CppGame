#include "Obstacle.h"
#include "../Graphics/Renderer.h"

Obstacle::Obstacle(int id, ObstacleType type, int x, int y)
    : Entity(id, EntityType::OBSTACLE, -1)
    , obstacleType(type)
{
    position = Vector2(x * 32.0f, y * 32.0f);
    
    switch (obstacleType) {
        case ObstacleType::TREE:
            maxHealth = 50;
            currentHealth = maxHealth;
            resourceYield = 10;
            width = 1;
            height = 1;
            spriteName = "tree";
            break;
        case ObstacleType::ROCK:
            maxHealth = 100;
            currentHealth = maxHealth;
            resourceYield = 5; // Metal from stone
            width = 1;
            height = 1;
            spriteName = "rock";
            break;
        case ObstacleType::BUSH:
            maxHealth = 20;
            currentHealth = maxHealth;
            resourceYield = 0;
            width = 1;
            height = 1;
            spriteName = "bush";
            break;
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


ResourceType Obstacle::GetResourceType() const {
    switch (obstacleType) {
        case ObstacleType::TREE: return ResourceType::WOOD;
        case ObstacleType::ROCK: return ResourceType::METAL;
        default: return ResourceType::NONE;
    }
}

void Obstacle::TakeHarvestDamage(int amount) {
    TakeDamage(amount);
}

void Obstacle::Serialize(SaveSystem* saveSystem) {
    Entity::Serialize(saveSystem);
    // Save obstacle type and resource yield
}

void Obstacle::Deserialize(SaveSystem* saveSystem) {
    Entity::Deserialize(saveSystem);
    // Load obstacle type and resource yield
}