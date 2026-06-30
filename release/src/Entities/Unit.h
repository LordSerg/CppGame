#ifndef UNIT_H
#define UNIT_H

#include "Entity.h"
#include "../Map/Pathfinding.h"
#include <queue>
#include <memory>

enum class UnitState {
    IDLE,
    MOVING,
    MINING,
    BUILDING,
    ATTACKING,
    CAPTURING,
    REMOVING_OBSTACLE,
    DEAD
};

enum class UnitType {
    PEASANT,
    WARRIOR,
    VETERAN,
    PALADIN
};

enum class ResourceType {
    NONE,
    WOOD,
    METAL,
    FOOD
};

class Command;
class Building;

class Unit : public Entity {
public:
    Unit(int id, int ownerId, UnitType unitType);
    virtual ~Unit();
    
    void Update(float deltaTime) override;
    void Render(class Renderer* renderer) override;
    
    // Command management
    void GiveCommand(std::shared_ptr<Command> command, bool queue = false);
    void ClearCommands();
    void StopCurrentAction();
    
    // Movement
    void MoveTo(const Point2D& destination);
    void SetPath(const std::vector<Point2D>& newPath);
    bool IsMoving() const { return state == UnitState::MOVING; }
    
    // Combat
    void Attack(Entity* target);
    void SetAttackTarget(Entity* target) { attackTarget = target; }
    Entity* GetAttackTarget() const { return attackTarget; }
    int GetAttackDamage() const { return attackDamage; }
    int GetAttackRange() const { return attackRange; }
    float GetAttackSpeed() const { return attackSpeed; } // attacks per second
    
    // Resource gathering
    void GatherResource(ResourceType resType, const Point2D& location);
    bool IsCarryingResource() const { return carryingResource != ResourceType::NONE; }
    ResourceType GetCarriedResource() const { return carryingResource; }
    int GetCarriedAmount() const { return carriedAmount; }
    
    // Building
    void BuildStructure(Building* building);
    Building* GetBuildingTarget() const { return buildingTarget; }
    
    // Obstacle removal
    void RemoveObstacle(class Obstacle* obstacle);
    
    // Capturing
    void CaptureBuilding(Building* building);
    
    // Stats
    UnitType GetUnitType() const { return unitType; }
    UnitState GetState() const { return state; }
    float GetSpeed() const { return speed; }
    int GetArmor() const { return armor; }
    int GetVisionRange() const { return visionRange; }
    
    // Upgrades
    void UpgradeToWarrior();
    void UpgradeToVeteran();
    void UpgradeToPaladin();
    
    // Apply tech upgrades
    void ApplyHealthUpgrade(int level);
    void ApplyToolsUpgrade(int level);
    void ApplyShieldsUpgrade(int level);
    void ApplySwordsUpgrade(int level);
    
    // Serialization
    void Serialize(class SaveSystem* saveSystem) override;
    void Deserialize(class SaveSystem* saveSystem) override;
    
protected:
    UnitType unitType;
    UnitState state;
    
    // Movement
    std::vector<Point2D> path;
    int currentPathIndex;
    float speed; // tiles per second
    Vector2 targetPosition;
    
    // Combat
    int attackDamage;
    int attackRange;
    float attackSpeed;
    float timeSinceLastAttack;
    Entity* attackTarget;
    int armor;
    
    // Vision
    int visionRange;
    
    // Commands
    std::queue<std::shared_ptr<Command>> commandQueue;
    std::shared_ptr<Command> currentCommand;
    
    // Resource gathering
    ResourceType carryingResource;
    int carriedAmount;
    int maxCarryAmount;
    Point2D resourceLocation;
    
    // Building
    Building* buildingTarget;
    float buildProgress;
    float buildSpeed; // progress per second
    
    // Capturing
    Building* captureTarget;
    float captureProgress;
    
    void UpdateMovement(float deltaTime);
    void UpdateCombat(float deltaTime);
    void UpdateMining(float deltaTime);
    void UpdateBuilding(float deltaTime);
    void UpdateCapturing(float deltaTime);
    void UpdateObstacleRemoval(float deltaTime);
    
    void ProcessNextCommand();
    void ReturnResourcesToStorage();
};

#endif // UNIT_H