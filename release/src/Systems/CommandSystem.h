#ifndef COMMANDSYSTEM_H
#define COMMANDSYSTEM_H

#include "../Utils/Math.h"
#include "../Entities/Unit.h"
#include "../Entities/Building.h"
#include <memory>
#include <vector>

enum class CommandType {
    MOVE,
    ATTACK,
    GATHER_RESOURCE,
    BUILD,
    CAPTURE,
    REMOVE_OBSTACLE,
    STOP
};

class Command {
public:
    virtual ~Command() = default;
    virtual CommandType GetType() const = 0;
    virtual void Execute(Unit* unit, class Map* map) = 0;
    virtual bool IsComplete(Unit* unit) const = 0;
    virtual void Update(Unit* unit, float deltaTime, class Map* map) = 0;
};

class MoveCommand : public Command {
public:
    MoveCommand(const Point2D& destination);
    CommandType GetType() const override { return CommandType::MOVE; }
    void Execute(Unit* unit, class Map* map) override;
    bool IsComplete(Unit* unit) const override;
    void Update(Unit* unit, float deltaTime, class Map* map) override;
    
private:
    Point2D destination;
};

class AttackCommand : public Command {
public:
    AttackCommand(Entity* target);
    CommandType GetType() const override { return CommandType::ATTACK; }
    void Execute(Unit* unit, class Map* map) override;
    bool IsComplete(Unit* unit) const override;
    void Update(Unit* unit, float deltaTime, class Map* map) override;
    
private:
    int targetId;
};

class GatherResourceCommand : public Command {
public:
    GatherResourceCommand(ResourceType type, const Point2D& location);
    CommandType GetType() const override { return CommandType::GATHER_RESOURCE; }
    void Execute(Unit* unit, class Map* map) override;
    bool IsComplete(Unit* unit) const override;
    void Update(Unit* unit, float deltaTime, class Map* map) override;
    
private:
    ResourceType resourceType;
    Point2D resourceLocation;
};

class BuildCommand : public Command {
public:
    BuildCommand(BuildingType type, const Point2D& location);
    CommandType GetType() const override { return CommandType::BUILD; }
    void Execute(Unit* unit, class Map* map) override;
    bool IsComplete(Unit* unit) const override;
    void Update(Unit* unit, float deltaTime, class Map* map) override;
    
private:
    BuildingType buildingType;
    Point2D buildLocation;
    int buildingId;
};

class CaptureCommand : public Command {
public:
    CaptureCommand(Building* target);
    CommandType GetType() const override { return CommandType::CAPTURE; }
    void Execute(Unit* unit, class Map* map) override;
    bool IsComplete(Unit* unit) const override;
    void Update(Unit* unit, float deltaTime, class Map* map) override;
    
private:
    int buildingId;
};

class RemoveObstacleCommand : public Command {
public:
    RemoveObstacleCommand(const Point2D& location);
    CommandType GetType() const override { return CommandType::REMOVE_OBSTACLE; }
    void Execute(Unit* unit, class Map* map) override;
    bool IsComplete(Unit* unit) const override;
    void Update(Unit* unit, float deltaTime, class Map* map) override;
    
private:
    Point2D obstacleLocation;
};

class CommandSystem {
public:
    CommandSystem(class Map* map);
    
    void Update(float deltaTime);
    
    // Issue commands to units
    void IssueMove(const std::vector<Unit*>& units, const Point2D& destination, bool queue = false);
    void IssueAttack(const std::vector<Unit*>& units, Entity* target, bool queue = false);
    void IssueGather(const std::vector<Unit*>& units, const Point2D& location, bool queue = false);
    void IssueBuild(Unit* peasant, BuildingType type, const Point2D& location);
    void IssueCapture(const std::vector<Unit*>& units, Building* building, bool queue = false);
    void IssueRemoveObstacle(Unit* peasant, const Point2D& location);
    void IssueStop(const std::vector<Unit*>& units);
    
private:
    class Map* map;
    
    void GiveCommandToUnit(Unit* unit, std::shared_ptr<Command> command, bool queue);
};

#endif // COMMANDSYSTEM_H