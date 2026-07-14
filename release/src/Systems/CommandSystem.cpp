#include "CommandSystem.h"
#include "../Map/Map.h"
#include "../Map/Pathfinding.h"
#include "../Entities/Building.h"
#include <set>

// Helper: generate formation positions around a center point
static std::vector<Point2D> GenerateFormationPositions(const Point2D& center, int count, Map* map) {
    std::vector<Point2D> positions;
    if (count <= 0) return positions;
    
    // The center position goes to the first unit
    positions.push_back(center);
    if (count == 1) return positions;
    
    // Spiral/box pattern: generate positions in concentric rings around the center
    // Ring 1: 8 neighbors (3x3 box minus center)
    // Ring 2: 16 neighbors (5x5 box minus inner 3x3)
    // etc.
    
    // We generate positions in order of distance from center
    std::set<Point2D> usedPositions;
    usedPositions.insert(center);
    
    int ring = 1;
    while ((int)positions.size() < count) {
        // Generate positions for this ring (a box of size (2*ring+1)x(2*ring+1) minus inner box)
        int boxSize = 2 * ring + 1;
        int startX = center.x - ring;
        int startY = center.y - ring;
        
        std::vector<Point2D> ringPositions;
        
        for (int dy = 0; dy < boxSize; dy++) {
            for (int dx = 0; dx < boxSize; dx++) {
                int px = startX + dx;
                int py = startY + dy;
                
                Point2D pos(px, py);
                
                // Skip if it's inside the inner box (already covered by previous rings)
                int distFromCenter = std::max(abs(px - center.x), abs(py - center.y));
                if (distFromCenter < ring) continue;
                
                // Skip if already used
                if (usedPositions.find(pos) != usedPositions.end()) continue;
                
                usedPositions.insert(pos);
                
                // Check if this tile is walkable and not occupied by non-ally entities
                if (map && map->IsInBounds(px, py) && map->IsWalkable(px, py)) {
                    ringPositions.push_back(pos);
                }
            }
        }
        
        // Sort ring positions by angle from center for a nice visual spread
        std::sort(ringPositions.begin(), ringPositions.end(), 
            [&center](const Point2D& a, const Point2D& b) {
                float angleA = atan2(a.y - center.y, a.x - center.x);
                float angleB = atan2(b.y - center.y, b.x - center.x);
                return angleA < angleB;
            });
        
        // Add to result
        for (const auto& pos : ringPositions) {
            if ((int)positions.size() >= count) break;
            positions.push_back(pos);
        }
        
        ring++;
        
        // Safety: limit ring size
        if (ring > 20) break;
    }
    
    return positions;
}

// MoveCommand implementation
MoveCommand::MoveCommand(const Point2D& dest)
    : destination(dest)
{
}

void MoveCommand::Execute(Unit* unit, Map* map) {
    if (!unit || !map) return;
    
    // Convert grid destination to world position
    Vector2 destPos(destination.x * 32.0f + 16.0f, destination.y * 32.0f + 16.0f);
    
    // Use NavMesh pathfinding
    unit->MoveToPosition(destPos);
}

bool MoveCommand::IsComplete(Unit* unit) const {
    return unit->GetNavPath().IsComplete() || !unit->IsMoving();
}

void MoveCommand::Update(Unit* unit, float deltaTime, Map* map) {
    // Movement is handled by unit's UpdateMovement
}

// AttackCommand implementation
AttackCommand::AttackCommand(Entity* target)
    : targetId(target ? target->GetId() : -1)
{
}

void AttackCommand::Execute(Unit* unit, Map* map) {
    if (!unit || !map) return;
    
    Entity* target = map->GetEntityById(targetId);
    if (target && target->IsAlive()) {
        unit->Attack(target);
    }
}

bool AttackCommand::IsComplete(Unit* unit) const {
    Entity* target = unit->GetAttackTarget();
    return !target || !target->IsAlive();
}

void AttackCommand::Update(Unit* unit, float deltaTime, Map* map) {
    Entity* target = map->GetEntityById(targetId);
    if (!target || !target->IsAlive()) {
        // Target is dead or gone
        return;
    }
    
    // Check if unit needs to move closer
    float distance = unit->GetPosition().Distance(target->GetPosition());
    if (distance > unit->GetAttackRange() * 32.0f) {
        // Move closer to target
        Point2D targetPos = target->GetGridPosition();
        Point2D unitPos = unit->GetGridPosition();
        
        std::vector<Point2D> path = Pathfinding::FindPath(map, unitPos, targetPos, 1, unit->GetId());
        if (!path.empty()) {
            unit->SetPath(path);
        }
    }
}

// GatherResourceCommand implementation
GatherResourceCommand::GatherResourceCommand(ResourceType type, const Point2D& location)
    : resourceType(type)
    , resourceLocation(location)
{
}

void GatherResourceCommand::Execute(Unit* unit, Map* map) {
    if (!unit || !map) return;
    
    unit->GatherResource(resourceType, resourceLocation);
}

bool GatherResourceCommand::IsComplete(Unit* unit) const {
    // Resource gathering is continuous
    return false;
}

void GatherResourceCommand::Update(Unit* unit, float deltaTime, Map* map) {
    // Unit handles resource gathering
}

// BuildCommand implementation
BuildCommand::BuildCommand(BuildingType type, const Point2D& location)
    : buildingType(type)
    , buildLocation(location)
    , buildingId(-1)
{
}

void BuildCommand::Execute(Unit* unit, Map* map) {
    if (!unit || !map) return;
    
    // Create blueprint building
    int id = 1000 + rand() % 9000; // Temporary ID generation
    auto building = std::make_shared<Building>(id, unit->GetOwnerId(), buildingType);
    building->SetPosition(buildLocation.x * 32.0f, buildLocation.y * 32.0f);
    building->StartConstruction();
    
    buildingId = id;
    map->AddEntity(building);
    
    unit->BuildStructure(building.get());
}

bool BuildCommand::IsComplete(Unit* unit) const {
    Building* building = unit->GetBuildingTarget();
    return !building || building->IsCompleted();
}

void BuildCommand::Update(Unit* unit, float deltaTime, Map* map) {
    // Unit handles building
}

// CaptureCommand implementation
CaptureCommand::CaptureCommand(Building* target)
    : buildingId(target ? target->GetId() : -1)
{
}

void CaptureCommand::Execute(Unit* unit, Map* map) {
    if (!unit || !map) return;
    
    Entity* entity = map->GetEntityById(buildingId);
    if (entity && entity->GetType() == EntityType::BUILDING) {
        Building* building = static_cast<Building*>(entity);
        unit->CaptureBuilding(building);
        building->AddCapturingUnit(unit->GetId());
    }
}

bool CaptureCommand::IsComplete(Unit* unit) const {
    return false; // Capturing is handled by the building
}

void CaptureCommand::Update(Unit* unit, float deltaTime, Map* map) {
    // Building handles capture progress
}

// RemoveObstacleCommand implementation
RemoveObstacleCommand::RemoveObstacleCommand(const Point2D& location)
    : obstacleLocation(location)
{
}

void RemoveObstacleCommand::Execute(Unit* unit, Map* map) {
    if (!unit || !map) return;
    
    // Find obstacle at location
    auto obstacles = map->GetObstaclesInRect(
        Rect(obstacleLocation.x, obstacleLocation.y, 1, 1)
    );
    
    if (!obstacles.empty()) {
        unit->RemoveObstacle(obstacles[0]);
    }
}

bool RemoveObstacleCommand::IsComplete(Unit* unit) const {
    Entity* target = unit->GetAttackTarget();
    return !target || !target->IsAlive();
}

void RemoveObstacleCommand::Update(Unit* unit, float deltaTime, Map* map) {
    // Unit handles obstacle removal
}

// CommandSystem implementation
CommandSystem::CommandSystem(Map* map)
    : map(map)
{
}

void CommandSystem::Update(float deltaTime) {
    // Commands are updated by units themselves
}

void CommandSystem::IssueMove(const std::vector<Unit*>& units, 
                             const Point2D& destination, bool queue) {
    if (units.empty()) return;
    
    // Convert to world position
    Vector2 centerPos(destination.x * 32.0f + 16.0f, 
                     destination.y * 32.0f + 16.0f);
    
    if (units.size() == 1) {
        // Single unit - go directly
        units[0]->MoveToPosition(centerPos);
    } else {
        // Multiple units - form a spread formation
        int cols = (int)std::ceil(std::sqrt((float)units.size()));
        int rows = (units.size() + cols - 1) / cols;
        
        float spacing = 48.0f; // NOT grid-aligned!
        
        float offsetX = -(cols - 1) * spacing * 0.5f;
        float offsetY = -(rows - 1) * spacing * 0.5f;
        
        int idx = 0;
        for (int r = 0; r < rows && idx < units.size(); r++) {
            for (int c = 0; c < cols && idx < units.size(); c++) {
                Vector2 formationPos(
                    centerPos.x + offsetX + c * spacing,
                    centerPos.y + offsetY + r * spacing
                );
                
                units[idx]->MoveToPosition(formationPos);
                idx++;
            }
        }
    }
}

void CommandSystem::IssueAttack(const std::vector<Unit*>& units, 
                               Entity* target, bool queue) {
    for (Unit* unit : units) {
        auto command = std::make_shared<AttackCommand>(target);
        GiveCommandToUnit(unit, command, queue);
    }
}

void CommandSystem::IssueGather(const std::vector<Unit*>& units, 
                               const Point2D& location, bool queue) {
    for (Unit* unit : units) {
        if (unit->GetUnitType() == UnitType::PEASANT) {
            ResourceType resType = ResourceType::WOOD; // Determine based on location
            auto command = std::make_shared<GatherResourceCommand>(resType, location);
            GiveCommandToUnit(unit, command, queue);
        }
    }
}

void CommandSystem::IssueBuild(Unit* peasant, BuildingType type, 
                              const Point2D& location) {
    if (peasant && peasant->GetUnitType() == UnitType::PEASANT) {
        auto command = std::make_shared<BuildCommand>(type, location);
        GiveCommandToUnit(peasant, command, false);
    }
}

void CommandSystem::IssueCapture(const std::vector<Unit*>& units, 
                                Building* building, bool queue) {
    for (Unit* unit : units) {
        auto command = std::make_shared<CaptureCommand>(building);
        GiveCommandToUnit(unit, command, queue);
    }
}

void CommandSystem::IssueRemoveObstacle(Unit* peasant, const Point2D& location) {
    if (peasant && peasant->GetUnitType() == UnitType::PEASANT) {
        auto command = std::make_shared<RemoveObstacleCommand>(location);
        GiveCommandToUnit(peasant, command, false);
    }
}

void CommandSystem::IssueStop(const std::vector<Unit*>& units) {
    for (Unit* unit : units) {
        unit->ClearCommands();
    }
}

void CommandSystem::GiveCommandToUnit(Unit* unit, std::shared_ptr<Command> command, 
                                     bool queue) {
    if (unit) {
        unit->GiveCommand(command, queue);
        command->Execute(unit, map);
    }
}