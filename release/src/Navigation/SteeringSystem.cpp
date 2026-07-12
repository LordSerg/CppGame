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
    
    // Cohesion (stay with group)
    forces.cohesion = Cohere(position, nearbyUnits, 96.0f);
    
    // Alignment (match group velocity)
    forces.alignment = Align(velocity, nearbyUnits, 64.0f);
    
    // Obstacle avoidance
    forces.avoidance = AvoidObstacles(position, velocity, 80.0f);
    
    return forces;
}

Vector2 SteeringSystem::Seek(const Vector2& position, const Vector2& target,
                             float maxSpeed) {
    Vector2 desired = target - position;
    float distance = desired.Length();
    
    if (distance < 4.0f) {
        return Vector2(0, 0); // Close enough
    }
    
    desired = desired.Normalized() * maxSpeed;
    return desired;
}

Vector2 SteeringSystem::Flee(const Vector2& position, const Vector2& threat) {
    Vector2 desired = position - threat;
    return desired.Normalized();
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
            float strength = 1.0f - (distance / separationRadius);
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
        return (center - position).Normalized();
    }
    
    return Vector2(0, 0);
}

Vector2 SteeringSystem::Align(const Vector2& velocity,
                              const std::vector<Unit*>& nearbyUnits,
                              float alignmentRadius) {
    Vector2 avgVelocity(0, 0);
    int count = 0;
    
    for (Unit* other : nearbyUnits) {
        avgVelocity = avgVelocity + other->GetVelocity();
        count++;
    }
    
    if (count > 0) {
        avgVelocity = avgVelocity * (1.0f / count);
        return (avgVelocity - velocity).Normalized();
    }
    
    return Vector2(0, 0);
}

Vector2 SteeringSystem::AvoidObstacles(const Vector2& position,
                                       const Vector2& velocity,
                                       float lookAheadDistance) {
    if (velocity.Length() < 0.01f) return Vector2(0, 0);
    
    Vector2 forward = velocity.Normalized();
    Vector2 lookAhead = position + forward * lookAheadDistance;
    
    // Check if lookahead point is on walkable navmesh
    int nodeId = navMesh->GetNodeAt(lookAhead);
    
    if (nodeId < 0) {
        // Obstacle ahead - steer away
        // Find nearest valid point
        const NavNode* nearestNode = nullptr;
        float minDist = 999999.0f;
        
        // Search nearby nodes
        auto nearbyNodes = navMesh->GetNodesInRadius(position, lookAheadDistance * 1.5f);
        
        for (int id : nearbyNodes) {
            const NavNode* node = navMesh->GetNode(id);
            if (!node) continue;
            
            Vector2 nodeCenter(node->center.x, node->center.y);
            float dist = lookAhead.Distance(nodeCenter);
            if (dist < minDist) {
                minDist = dist;
                nearestNode = node;
            }
        }
        
        if (nearestNode) {
            Vector2 steerTarget(nearestNode->center.x, nearestNode->center.y);
            Vector2 avoidance = (steerTarget - position).Normalized();
            
            // Add perpendicular component
            Vector2 perpendicular(-forward.y, forward.x);
            float dot = avoidance.x * perpendicular.x + avoidance.y * perpendicular.y;
            
            if (std::abs(dot) < 0.3f) {
                avoidance = avoidance + perpendicular * (dot > 0 ? 0.5f : -0.5f);
            }
            
            return avoidance;
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
    
    // Square formation
    int cols = (int)std::ceil(std::sqrt((float)count));
    int rows = (count + cols - 1) / cols;
    
    float offsetX = -(cols - 1) * spacing * 0.5f;
    float offsetY = -(rows - 1) * spacing * 0.5f;
    
    int idx = 0;
    for (int r = 0; r < rows && idx < count; r++) {
        for (int c = 0; c < cols && idx < count; c++) {
            positions.push_back(Vector2(
                center.x + offsetX + c * spacing,
                center.y + offsetY + r * spacing
            ));
            idx++;
        }
    }
    
    return positions;
}

std::vector<Vector2> SteeringSystem::CalculateFlowField(
    const Vector2& destination,
    const std::vector<Unit*>& units) {
    
    std::vector<Vector2> flowDirections;
    
    for (Unit* unit : units) {
        Vector2 toDest = destination - unit->GetPosition();
        flowDirections.push_back(toDest.Normalized());
    }
    
    return flowDirections;
}