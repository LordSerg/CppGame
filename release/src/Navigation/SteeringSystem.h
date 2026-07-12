#ifndef STEERINGSYSTEM_H
#define STEERINGSYSTEM_H

#include "../Utils/Math.h"
#include <vector>
#include <memory>

class Unit;
class NavMesh;
struct NavPath;

struct SteeringForces {
    Vector2 seek;
    Vector2 separation;
    Vector2 cohesion;
    Vector2 alignment;
    Vector2 avoidance;
    
    SteeringForces() 
        : seek(0, 0), separation(0, 0), cohesion(0, 0), 
          alignment(0, 0), avoidance(0, 0) {}
    
    Vector2 GetTotal() const {
        return seek + separation * 1.5f + cohesion * 0.5f + 
               alignment * 0.3f + avoidance * 2.0f;
    }
};

class SteeringSystem {
public:
    SteeringSystem(NavMesh* navMesh);
    
    // Calculate steering forces for a unit
    SteeringForces CalculateSteering(Unit* unit, 
                                     const std::vector<Unit*>& nearbyUnits,
                                     float deltaTime);
    
    // Formation movement
    std::vector<Vector2> CalculateFormationPositions(
        const Vector2& center, int count, float spacing = 48.0f);

    Vector2 Flee(const Vector2& position, const Vector2& threat);
    std::vector<Vector2> CalculateFlowField(const Vector2& destination, const std::vector<Unit*>& units);
    
private:
    NavMesh* navMesh;
    
    Vector2 Seek(const Vector2& position, const Vector2& target, 
                float maxSpeed);
    Vector2 Separate(const Vector2& position, 
                    const std::vector<Unit*>& nearbyUnits,
                    float separationRadius);
    Vector2 Cohere(const Vector2& position, 
                  const std::vector<Unit*>& nearbyUnits,
                  float cohesionRadius);
    Vector2 Align(const Vector2& velocity, 
                 const std::vector<Unit*>& nearbyUnits,
                 float alignmentRadius);
    Vector2 AvoidObstacles(const Vector2& position, 
                          const Vector2& velocity,
                          float lookAheadDistance);
};

#endif // STEERINGSYSTEM_H