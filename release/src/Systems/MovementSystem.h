#ifndef MOVEMENTSYSTEM_H
#define MOVEMENTSYSTEM_H

#include "../Utils/Math.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>

class Map;
class Unit;

struct UnitCell {
    std::vector<Unit*> units;
};

class MovementSystem {
public:
    MovementSystem(Map* map);
    
    void Update(float deltaTime);
    
    // Register/unregister units for collision detection
    void RegisterUnit(Unit* unit);
    void UnregisterUnit(Unit* unit);
    void UpdateUnitPosition(Unit* unit, const Vector2& oldPos, const Vector2& newPos);
    
    // Collision detection
    bool IsPositionBlocked(const Vector2& position, Unit* ignoredUnit = nullptr) const;
    std::vector<Unit*> GetUnitsInRadius(const Vector2& position, float radius, 
                                        Unit* ignoredUnit = nullptr) const;
    
    // Path planning with collision
    Vector2 GetAvoidanceVector(Unit* unit, const Vector2& desiredVelocity);
    bool CanMoveToPosition(Unit* unit, const Vector2& newPosition) const;
    
    // Formation movement
    void MoveUnitsInFormation(const std::vector<Unit*>& units, const Point2D& destination);
    
private:
    Map* map;
    
    // Spatial partitioning for efficient collision detection
    const int cellSize = 64; // pixels (2 tiles)
    std::unordered_map<int, UnitCell> spatialGrid;
    
    // Helper functions
    int GetCellKey(int x, int y) const;
    Point2D GetCellCoords(const Vector2& position) const;
    std::vector<Point2D> GetNearbyCells(const Point2D& cell) const;
    
    // Collision resolution
    Vector2 ResolveCollision(Unit* unit, const Vector2& desiredPosition);
    Vector2 CalculateRepulsionForce(Unit* unit, Unit* other);
    
    // Local avoidance (RVO - Reciprocal Velocity Obstacles)
    Vector2 ComputeNewVelocity(Unit* unit, const Vector2& preferredVelocity);
    
    // Formation
    std::vector<Point2D> CalculateFormationPositions(const Point2D& center, int count);
};

#endif // MOVEMENTSYSTEM_H