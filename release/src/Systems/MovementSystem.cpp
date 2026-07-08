#include "MovementSystem.h"
#include "../Map/Map.h"
#include "../Entities/Unit.h"
#include <algorithm>
#include <cmath>

MovementSystem::MovementSystem(Map* map)
    : map(map)
{
}

void MovementSystem::Update(float deltaTime) {
    // Movement is handled by individual units, but we update spatial grid
    // This is called after all units have updated their positions
}

void MovementSystem::RegisterUnit(Unit* unit) {
    if (!unit) return;
    
    Point2D cell = GetCellCoords(unit->GetPosition());
    int key = GetCellKey(cell.x, cell.y);
    
    spatialGrid[key].units.push_back(unit);
}

void MovementSystem::UnregisterUnit(Unit* unit) {
    if (!unit) return;
    
    Point2D cell = GetCellCoords(unit->GetPosition());
    int key = GetCellKey(cell.x, cell.y);
    
    auto& units = spatialGrid[key].units;
    units.erase(std::remove(units.begin(), units.end(), unit), units.end());
}

void MovementSystem::UpdateUnitPosition(Unit* unit, const Vector2& oldPos, 
                                       const Vector2& newPos) {
    if (!unit) return;
    
    Point2D oldCell = GetCellCoords(oldPos);
    Point2D newCell = GetCellCoords(newPos);
    
    if (oldCell.x != newCell.x || oldCell.y != newCell.y) {
        // Unit moved to a different cell
        int oldKey = GetCellKey(oldCell.x, oldCell.y);
        int newKey = GetCellKey(newCell.x, newCell.y);
        
        // Remove from old cell
        auto& oldUnits = spatialGrid[oldKey].units;
        oldUnits.erase(std::remove(oldUnits.begin(), oldUnits.end(), unit), 
                      oldUnits.end());
        
        // Add to new cell
        spatialGrid[newKey].units.push_back(unit);
    }
}

bool MovementSystem::IsPositionBlocked(const Vector2& position, Unit* ignoredUnit) const {
    const float unitRadius = 16.0f; // Half of tile size
    
    auto nearbyUnits = GetUnitsInRadius(position, unitRadius * 1.5f, ignoredUnit);
    
    for (Unit* other : nearbyUnits) {
        float distance = position.Distance(other->GetPosition());
        if (distance < unitRadius * 2.0f) {
            return true;
        }
    }
    
    // Check if position is on walkable terrain
    Point2D gridPos(position.x / 32, position.y / 32);
    if (!map->IsWalkable(gridPos.x, gridPos.y)) {
        return true;
    }
    
    return false;
}

std::vector<Unit*> MovementSystem::GetUnitsInRadius(const Vector2& position, float radius,
                                                     Unit* ignoredUnit) const {
    std::vector<Unit*> result;
    
    Point2D centerCell = GetCellCoords(position);
    int cellRadius = (radius / cellSize) + 1;
    
    for (int dy = -cellRadius; dy <= cellRadius; dy++) {
        for (int dx = -cellRadius; dx <= cellRadius; dx++) {
            int key = GetCellKey(centerCell.x + dx, centerCell.y + dy);
            
            auto it = spatialGrid.find(key);
            if (it != spatialGrid.end()) {
                for (Unit* unit : it->second.units) {
                    if (unit == ignoredUnit) continue;
                    
                    float distance = position.Distance(unit->GetPosition());
                    if (distance <= radius) {
                        result.push_back(unit);
                    }
                }
            }
        }
    }
    
    return result;
}

Vector2 MovementSystem::GetAvoidanceVector(Unit* unit, const Vector2& desiredVelocity) {
    if (!unit) return desiredVelocity;
    
    const float lookAheadTime = 0.5f; // seconds
    const float unitRadius = 16.0f;
    
    Vector2 currentPos = unit->GetPosition();
    Vector2 futurePos = currentPos + desiredVelocity * lookAheadTime;
    
    // Get nearby units
    auto nearbyUnits = GetUnitsInRadius(currentPos, unitRadius * 4.0f, unit);
    
    Vector2 avoidanceForce(0, 0);
    int avoidanceCount = 0;
    
    for (Unit* other : nearbyUnits) {
        Vector2 otherPos = other->GetPosition();
        Vector2 toOther = otherPos - currentPos;
        float distance = toOther.Length();
        
        if (distance < 0.01f) {
            // Units are on top of each other, push apart
            float angle = (rand() % 360) * 3.14159f / 180.0f;
            avoidanceForce = avoidanceForce + Vector2(cos(angle), sin(angle)) * 100.0f;
            avoidanceCount++;
            continue;
        }
        
        // Predict collision
        Vector2 otherVelocity(0, 0);
        if (other->IsMoving()) {
            // Estimate other's velocity based on their movement
            otherVelocity = Vector2(
                cos(atan2(toOther.y, toOther.x)) * other->GetSpeed() * 32.0f,
                sin(atan2(toOther.y, toOther.x)) * other->GetSpeed() * 32.0f
            );
        }
        
        Vector2 relativeVelocity = desiredVelocity - otherVelocity;
        Vector2 relativePosition = otherPos - currentPos;
        
        // Time to collision
        float timeToCollision = -1;
        float a = relativeVelocity.x * relativeVelocity.x + 
                  relativeVelocity.y * relativeVelocity.y;
        float b = 2 * (relativePosition.x * relativeVelocity.x + 
                      relativePosition.y * relativeVelocity.y);
        float c = relativePosition.x * relativePosition.x + 
                  relativePosition.y * relativePosition.y - 
                  (unitRadius * 2) * (unitRadius * 2);
        
        float discriminant = b * b - 4 * a * c;
        
        if (discriminant >= 0 && a != 0) {
            timeToCollision = (-b - sqrt(discriminant)) / (2 * a);
        }
        
        if (timeToCollision > 0 && timeToCollision < lookAheadTime) {
            // Collision predicted, calculate avoidance
            Vector2 normal = relativePosition.Normalized();
            float strength = (lookAheadTime - timeToCollision) / lookAheadTime;
            avoidanceForce = avoidanceForce - normal * strength * 200.0f;
            avoidanceCount++;
        } else if (distance < unitRadius * 2.5f) {
            // Too close, push away
            Vector2 pushDirection = (currentPos - otherPos).Normalized();
            float strength = 1.0f - (distance / (unitRadius * 2.5f));
            avoidanceForce = avoidanceForce + pushDirection * strength * 150.0f;
            avoidanceCount++;
        }
    }
    
    if (avoidanceCount > 0) {
        // Combine desired velocity with avoidance
        Vector2 adjustedVelocity = desiredVelocity + avoidanceForce * 0.1f;
        
        // Maintain desired speed
        float desiredSpeed = desiredVelocity.Length();
        if (adjustedVelocity.Length() > 0.01f) {
            adjustedVelocity = adjustedVelocity.Normalized() * desiredSpeed;
        }
        
        return adjustedVelocity;
    }
    
    return desiredVelocity;
}

bool MovementSystem::CanMoveToPosition(Unit* unit, const Vector2& newPosition) const {
    if (!unit) return false;
    
    // Check terrain
    Point2D gridPos(newPosition.x / 32, newPosition.y / 32);
    if (!map->IsInBounds(gridPos.x, gridPos.y)) return false;
    if (!map->IsWalkable(gridPos.x, gridPos.y)) return false;
    
    // Check collision with other units
    const float unitRadius = 16.0f;
    auto nearbyUnits = GetUnitsInRadius(newPosition, unitRadius, unit);
    
    for (Unit* other : nearbyUnits) {
        float distance = newPosition.Distance(other->GetPosition());
        if (distance < unitRadius * 1.8f) { // Slightly overlapping allowed for smoother movement
            return false;
        }
    }
    
    return true;
}

Vector2 MovementSystem::ResolveCollision(Unit* unit, const Vector2& desiredPosition) {
    if (!unit) return desiredPosition;
    
    if (CanMoveToPosition(unit, desiredPosition)) {
        return desiredPosition;
    }
    
    // Try to find nearby valid position
    Vector2 currentPos = unit->GetPosition();
    Vector2 direction = (desiredPosition - currentPos).Normalized();
    
    // Try positions in a small radius around desired position
    const int attempts = 8;
    const float searchRadius = 32.0f;
    
    for (int i = 0; i < attempts; i++) {
        float angle = (i / (float)attempts) * 2.0f * 3.14159f;
        Vector2 offset(cos(angle) * searchRadius, sin(angle) * searchRadius);
        Vector2 testPos = desiredPosition + offset;
        
        if (CanMoveToPosition(unit, testPos)) {
            return testPos;
        }
    }
    
    // If no valid position found, try to move partially
    float stepSize = 8.0f;
    Vector2 partialMove = currentPos;
    
    for (float dist = 0; dist < desiredPosition.Distance(currentPos); dist += stepSize) {
        Vector2 testPos = currentPos + direction * dist;
        if (!CanMoveToPosition(unit, testPos)) {
            break;
        }
        partialMove = testPos;
    }
    
    return partialMove;
}

Vector2 MovementSystem::CalculateRepulsionForce(Unit* unit, Unit* other) {
    if (!unit || !other) return Vector2(0, 0);
    
    Vector2 diff = unit->GetPosition() - other->GetPosition();
    float distance = diff.Length();
    
    if (distance < 0.01f) {
        // Random direction if units are exactly on top of each other
        float angle = (rand() % 360) * 3.14159f / 180.0f;
        return Vector2(cos(angle), sin(angle)) * 100.0f;
    }
    
    const float minDistance = 32.0f;
    if (distance < minDistance) {
        float strength = (minDistance - distance) / minDistance;
        return diff.Normalized() * strength * 100.0f;
    }
    
    return Vector2(0, 0);
}

Vector2 MovementSystem::ComputeNewVelocity(Unit* unit, const Vector2& preferredVelocity) {
    // Simplified RVO (Reciprocal Velocity Obstacles) algorithm
    if (!unit) return preferredVelocity;
    
    const float maxSpeed = unit->GetSpeed() * 32.0f;
    const float unitRadius = 16.0f;
    
    auto nearbyUnits = GetUnitsInRadius(unit->GetPosition(), unitRadius * 5.0f, unit);
    
    if (nearbyUnits.empty()) {
        return preferredVelocity;
    }
    
    // Sample velocities in a circle
    const int samples = 16;
    Vector2 bestVelocity = preferredVelocity;
    float bestScore = -1e9f;
    
    for (int i = 0; i < samples; i++) {
        float angle = (i / (float)samples) * 2.0f * 3.14159f;
        
        for (float speed = maxSpeed; speed >= 0; speed -= maxSpeed / 4.0f) {
            Vector2 testVelocity(cos(angle) * speed, sin(angle) * speed);
            
            // Score this velocity
            float score = 0;
            
            // Prefer velocities close to preferred
            score += testVelocity.x * preferredVelocity.x + 
                    testVelocity.y * preferredVelocity.y;
            
            // Check for collisions with this velocity
            bool collision = false;
            for (Unit* other : nearbyUnits) {
                Vector2 relativePos = other->GetPosition() - unit->GetPosition();
                float distance = relativePos.Length();
                
                if (distance < unitRadius * 2.0f) {
                    // Too close, penalize heavily
                    score -= 1000.0f;
                }
                
                // Predict future collision
                Vector2 futurePos = unit->GetPosition() + testVelocity * 0.5f;
                Vector2 otherFuturePos = other->GetPosition();
                
                if (other->IsMoving()) {
                    // Estimate other's future position
                    Vector2 otherDir = (other->GetPosition() - unit->GetPosition()).Normalized();
                    otherFuturePos = otherFuturePos + otherDir * other->GetSpeed() * 16.0f;
                }
                
                float futureDistance = futurePos.Distance(otherFuturePos);
                if (futureDistance < unitRadius * 2.0f) {
                    score -= 500.0f;
                }
            }
            
            if (score > bestScore) {
                bestScore = score;
                bestVelocity = testVelocity;
            }
        }
    }
    
    return bestVelocity;
}

void MovementSystem::MoveUnitsInFormation(const std::vector<Unit*>& units, 
                                         const Point2D& destination) {
    if (units.empty()) return;
    
    // Calculate formation positions
    std::vector<Point2D> positions = CalculateFormationPositions(destination, units.size());
    
    // Assign each unit to nearest formation position
    std::vector<bool> assigned(positions.size(), false);
    
    for (Unit* unit : units) {
        Point2D unitPos = unit->GetGridPosition();
        float minDist = 1e9f;
        int bestIdx = -1;
        
        for (size_t i = 0; i < positions.size(); i++) {
            if (assigned[i]) continue;
            
            int dx = positions[i].x - unitPos.x;
            int dy = positions[i].y - unitPos.y;
            float dist = sqrt(dx * dx + dy * dy);
            
            if (dist < minDist) {
                minDist = dist;
                bestIdx = i;
            }
        }
        
        if (bestIdx >= 0) {
            assigned[bestIdx] = true;
            
            // Give movement command to this position
            std::vector<Point2D> path = Pathfinding::FindPath(
                map, unitPos, positions[bestIdx], 1
            );
            
            if (!path.empty()) {
                unit->SetPath(path);
            }
        }
    }
}

std::vector<Point2D> MovementSystem::CalculateFormationPositions(const Point2D& center, 
                                                                  int count) {
    std::vector<Point2D> positions;
    
    if (count <= 0) return positions;
    
    if (count == 1) {
        positions.push_back(center);
        return positions;
    }
    
    // Create a rectangular formation
    int cols = (int)ceil(sqrt(count));
    int rows = (count + cols - 1) / cols;
    
    int spacing = 2; // tiles between units
    int offsetX = -(cols * spacing) / 2;
    int offsetY = -(rows * spacing) / 2;
    
    int idx = 0;
    for (int row = 0; row < rows && idx < count; row++) {
        for (int col = 0; col < cols && idx < count; col++) {
            Point2D pos(
                center.x + offsetX + col * spacing,
                center.y + offsetY + row * spacing
            );
            positions.push_back(pos);
            idx++;
        }
    }
    
    return positions;
}

int MovementSystem::GetCellKey(int x, int y) const {
    // Hash function for spatial grid
    return (x & 0xFFFF) | ((y & 0xFFFF) << 16);
}

Point2D MovementSystem::GetCellCoords(const Vector2& position) const {
    return Point2D(
        (int)(position.x / cellSize),
        (int)(position.y / cellSize)
    );
}

std::vector<Point2D> MovementSystem::GetNearbyCells(const Point2D& cell) const {
    std::vector<Point2D> cells;
    
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            cells.push_back(Point2D(cell.x + dx, cell.y + dy));
        }
    }
    
    return cells;
}