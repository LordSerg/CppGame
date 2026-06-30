#ifndef OBSTACLE_H
#define OBSTACLE_H

#include "Entity.h"

enum class ObstacleType {
    ROCK,
    TREE
};

class Obstacle : public Entity {
public:
    Obstacle(int id, ObstacleType type, int x, int y);
    
    void Update(float deltaTime) override;
    void Render(class Renderer* renderer) override;
    
    ObstacleType GetObstacleType() const { return obstacleType; }
    
private:
    ObstacleType obstacleType;
};

#endif // OBSTACLE_H