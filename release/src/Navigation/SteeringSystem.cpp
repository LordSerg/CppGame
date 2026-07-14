#include "SteeringSystem.h"
#include "NavMesh.h"
#include "../Entities/Unit.h"
#include "../Map/Map.h"
#include <algorithm>
#include <cmath>

SteeringSystem::SteeringSystem(NavMesh* navMesh) : navMesh(navMesh) {}

SteeringForces SteeringSystem::CalculateSteering(
    Unit* unit, 
    const std::vector<Unit*>& nearbyUnits,
    float deltaTime) {
    
    SteeringForces forces;
    
    Vector2 position = unit->GetPosition();
    Vector2 velocity = unit->GetVelocity();
    float maxSpeed = unit->GetSpeed() * 32.0f;
    
    // Seek target waypoint
    const NavPath& path = unit->GetNavPath();
    if (!path.IsComplete()) {
        Vector2 target = path.GetCurrentWaypoint();
        forces.seek = Seek(position, target, maxSpeed);
    }
    
    // Separation (don't crowd neighbors)
    forces.separation = Separate(position, nearbyUnits, 48.0f);
    
    // Cohesion (stay with group) - but weaker
    forces.cohesion = Cohere(position, nearbyUnits, 96.0f);
    
    // Alignment (match group velocity) - but weaker
    forces.alignment = Align(velocity, nearbyUnits, 64.0f);
    
    // Obstacle avoidance
    forces.avoidance = AvoidObstacles(position, velocity, 60.0f);
    
    return forces;
}

Vector2 SteeringSystem::Seek(const Vector2& position, const Vector2& target,
                             float maxSpeed) {
    Vector2 desired = target - position;
    float distance = desired.Length();
    
    if (distance < 4.0f) {
        return Vector2(0, 0); // Close enough
    }
    
    // Slow down when approaching target
    if (distance < 50.0f) {
        float ratio = distance / 50.0f;
        desired = desired.Normalized() * (maxSpeed * ratio);
    } else {
        desired = desired.Normalized() * maxSpeed;
    }
    
    return desired;
}

Vector2 SteeringSystem::Separate(const Vector2& position,
                                 const std::vector<Unit*>& nearbyUnits,
                                 float separationRadius) {
    Vector2 force(0, 0);
    int count = 0;
    
    for (Unit* other : nearbyUnits) {
        Vector2 diff = position - other->GetPosition();
        float distance = diff.Length();
        
        if (distance > 0 && distance < separationRadius) {
            // Stronger repulsion when closer
            float strength = 1.0f - (distance / separationRadius);
            strength = strength * strength; // Square for stronger effect when close
            
            force = force + diff.Normalized() * strength;
            count++;
        }
    }
    
    if (count > 0) {
        force = force * (1.0f / count);
    }
    
    return force;
}

Vector2 SteeringSystem::Cohere(const Vector2& position,
                               const std::vector<Unit*>& nearbyUnits,
                               float cohesionRadius) {
    if (nearbyUnits.empty()) return Vector2(0, 0);
    
    Vector2 center(0, 0);
    int count = 0;
    
    for (Unit* other : nearbyUnits) {
        float distance = position.Distance(other->GetPosition());
        if (distance > 0 && distance < cohesionRadius) {
            center = center + other->GetPosition();
            count++;
        }
    }
    
    if (count > 0) {
        center = center * (1.0f / count);
        Vector2 desired = center - position;
        if (desired.Length() > 0.1f) {
            return desired.Normalized() * 0.5f; // Weak cohesion
        }
    }
    
    return Vector2(0, 0);
}

Vector2 SteeringSystem::Align(const Vector2& velocity,
                              const std::vector<Unit*>& nearbyUnits,
                              float alignmentRadius) {
    if (nearbyUnits.empty()) return Vector2(0, 0);
    
    Vector2 avgVelocity(0, 0);
    int count = 0;
    
    for (Unit* other : nearbyUnits) {
        avgVelocity = avgVelocity + other->GetVelocity();
        count++;
    }
    
    if (count > 0) {
        avgVelocity = avgVelocity * (1.0f / count);
        Vector2 desired = avgVelocity - velocity;
        if (desired.Length() > 0.1f) {
            return desired.Normalized() * 0.3f; // Weak alignment
        }
    }
    
    return Vector2(0, 0);
}

Vector2 SteeringSystem::AvoidObstacles(const Vector2& position,
                                       const Vector2& velocity,
                                       float lookAheadDistance) {
    if (!navMesh) return Vector2(0, 0);
    if (velocity.Length() < 0.01f) return Vector2(0, 0);
    
    Vector2 forward = velocity.Normalized();
    Vector2 lookAhead = position + forward * lookAheadDistance;
    
    // Check if lookahead point is on walkable polygon
    int currentPoly = navMesh->GetPolygonAt(position);
    int aheadPoly = navMesh->GetPolygonAt(lookAhead);
    
    if (currentPoly >= 0 && aheadPoly < 0) {
        // We're heading toward non-walkable area - steer away
        
        // Sample points around the forward direction to find a clear path
        const int samples = 5;
        Vector2 bestDirection = forward;
        float bestScore = -1.0f;
        
        for (int i = 0; i < samples; i++) {
            float angleOffset = (i - samples / 2) * 0.3f; // ±45 degrees
            float angle = atan2(forward.y, forward.x) + angleOffset;
            
            Vector2 testDirection(cos(angle), sin(angle));
            Vector2 testPoint = position + testDirection * lookAheadDistance;
            
            int testPoly = navMesh->GetPolygonAt(testPoint);
            
            if (testPoly >= 0) {
                // This direction is clear
                float score = 1.0f - std::abs(angleOffset);
                if (score > bestScore) {
                    bestScore = score;
                    bestDirection = testDirection;
                }
            }
        }
        
        if (bestScore > 0) {
            // Found a clear direction
            return (bestDirection - forward) * 2.0f;
        } else {
            // No clear direction found - turn perpendicular
            Vector2 perpendicular(-forward.y, forward.x);
            return perpendicular * 2.0f;
        }
    }
    
    return Vector2(0, 0);
}

std::vector<Vector2> SteeringSystem::CalculateFormationPositions(
    const Vector2& center, int count, float spacing) {
    std::vector<Vector2> positions;
    
    if (count <= 0) return positions;
    if (count == 1) {
        positions.push_back(center);
        return positions;
    }
    
    // Square formation with non-grid-aligned spacing
    int cols = (int)std::ceil(std::sqrt((float)count));
    int rows = (count + cols - 1) / cols;
    
    float offsetX = -(cols - 1) * spacing * 0.5f;
    float offsetY = -(rows - 1) * spacing * 0.5f;
    
    int idx = 0;
    for (int r = 0; r < rows && idx < count; r++) {
        for (int c = 0; c < cols && idx < count; c++) {
            // Add small random offset to break perfect grid
            float jitterX = (rand() % 10 - 5) * 0.5f;
            float jitterY = (rand() % 10 - 5) * 0.5f;
            
            positions.push_back(Vector2(
                center.x + offsetX + c * spacing + jitterX,
                center.y + offsetY + r * spacing + jitterY
            ));
            idx++;
        }
    }
    
    return positions;
}