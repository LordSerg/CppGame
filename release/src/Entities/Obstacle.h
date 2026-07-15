#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "Entity.h"
#include "Unit.h"
#include <string>

enum class ObstacleType {
    ROCK,
    TREE,
    BUSH
};

class Obstacle : public Entity {
public:
    Obstacle(int id, ObstacleType type, int x, int y);
    
    void Update(float deltaTime) override;
    void Render(class Renderer* renderer) override;
    
    ObstacleType GetObstacleType() const { return obstacleType; }

    // Resource yield when destroyed/harvested
    int GetResourceYield() const { return resourceYield; }
    ResourceType GetResourceType() const;

    // Harvesting
    void TakeHarvestDamage(int amount);
    bool IsHarvestable() const { return currentHealth > 0; }

    // Serialization
    void Serialize(class SaveSystem* saveSystem) override;
    void Deserialize(class SaveSystem* saveSystem) override;
    
private:
    ObstacleType obstacleType;
    int resourceYield;
    void InitFromType();
};

#endif // OBSTACLE_H