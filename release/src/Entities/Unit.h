#ifndef UNIT_H
#define UNIT_H

#include "Entity.h"
#include "../Map/Pathfinding.h"
#include "../Navigation/NavMesh.h"
#include "../Navigation/SteeringSystem.h"
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
    
    
    // NavMesh-based movement
    void MoveToPosition(const Vector2& worldPosition);
    void MoveToTile(const Point2D& tile);
    
    // Update these
    const NavPath& GetNavPath() const { return navPath; }
    void SetNavPath(const NavPath& path);
    Vector2 GetVelocity() const { return velocity; }
    void SetSteeringSystem(SteeringSystem* ss) { steeringSystem = ss; }

    void MoveTo(const Point2D& destination);
    void SetPath(const std::vector<Point2D>& newPath);
    bool IsMoving() const { return state == UnitState::MOVING; }
    void CalculatePathTo(const Vector2& destination);
    void UpdateNearbyUnits(const std::vector<Unit*>& allUnits);
    
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
    
    // Set the map reference for collision avoidance
    void SetMap(class Map* m) { mapRef = m; }
    
    // Serialization
    void Serialize(class SaveSystem* saveSystem) override;
    void Deserialize(class SaveSystem* saveSystem) override;

    void SetMovementSystem(class MovementSystem* ms) { movementSystem = ms; }

    const std::vector<Point2D>& GetPath() const { return path; }

    void HandleCollision(Unit* otherUnit, float deltaTime);
    bool IsCollidingWith(Unit* otherUnit) const;

    //bool IsWaitingForPath() const { return isWaitingForPath; }
    //void SetWaitingForPath(bool waiting) { isWaitingForPath = waiting; }
    //const std::vector<Point2D>& GetPath() const { return path; }
    
    std::vector<Unit*> GetAllUnits(); // Helper to get all units from map
    
    // Velocity smoothing
    void UpdateVelocity(float deltaTime, const Vector2& targetVelocity);

private:
    // Collision avoidance helpers
    Vector2 ApplyCollisionAvoidance(const Vector2& desiredVelocity, float deltaTime);
    bool ValidatePosition(const Vector2& pos) const;
    Vector2 TrySlideMovement(const Vector2& desiredVelocity, float deltaTime);
    bool TryRepath();
    void ApplyStuckRecovery(float deltaTime);
    
    class MovementSystem* movementSystem; // Reference to movement system

    bool IsTileOccupiedByOtherUnit(const Point2D& tile) const;
    Unit* GetUnitAtTile(const Point2D& tile) const;
    Vector2 ApplySmartAvoidance(const Vector2& desiredVelocity, float deltaTime);
    bool TryAlternativeMovement(const Vector2& desiredVelocity, float deltaTime);
    bool TrySimpleSlide(const Vector2& desiredVelocity, float deltaTime);
    
    Unit* FindBlockingUnit(const Vector2& targetPosition) const;
    
    float collisionWaitTimer;
    int collidingWithUnitId;
    bool shouldStepAside;

    NavPath navPath;
    SteeringSystem* steeringSystem;
    Vector2 velocity;
    Vector2 smoothedVelocity;
    
    //static const float NEARBY_UPDATE_INTERVAL = 0.5f;
    static const float NEARBY_UPDATE_INTERVAL;
    
    // Nearby unit cache
    std::vector<Unit*> nearbyUnits;
    float nearbyUpdateTimer = 0.0f;
protected:
    UnitType unitType;
    UnitState state;
    
    // Movement
    std::vector<Point2D> path;
    int currentPathIndex;
    float speed; // tiles per second
    Vector2 targetPosition;
    
    // Periodic repathing
    float repathTimer;
    static const float REPATH_INTERVAL; // recalculate path every N seconds
    
    // Stuck detection
    float stuckTimer;
    Vector2 lastStuckCheckPosition;
    static const float STUCK_TIMEOUT; // seconds before considering unit stuck
    static const float STUCK_FORCE_REPATH_TIMEOUT; // seconds before forcing repath
    
    // Map reference for collision avoidance
    class Map* mapRef;
    
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