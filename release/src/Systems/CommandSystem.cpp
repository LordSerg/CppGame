#include "CommandSystem.h"
#include "../Map/Map.h"
#include "../Map/Pathfinding.h"
#include "../Entities/Building.h"

// MoveCommand implementation
MoveCommand::MoveCommand(const Point2D& dest)
    : destination(dest)
{
}

void MoveCommand::Execute(Unit* unit, Map* map) {
    if (!unit || !map) return;
    
    Point2D start = unit->GetGridPosition();
    std::vector<Point2D> path = Pathfinding::FindPath(map, start, destination, 1);
    
    if (!path.empty()) {
        unit->SetPath(path);
    }
}

bool MoveCommand::IsComplete(Unit* unit) const {
    return !unit->IsMoving() || unit->GetGridPosition() == destination;
}

void MoveCommand::Update(Unit* unit, float deltaTime, Map* map) {
    // Movement is handled by unit itself
    if (IsComplete(unit)) {
        // Command complete
    }
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
        
        std::vector<Point2D> path = Pathfinding::FindPath(map, unitPos, targetPos, 1);
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
    for (Unit* unit : units) {
        auto command = std::make_shared<MoveCommand>(destination);
        GiveCommandToUnit(unit, command, queue);
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