#include "Unit.h"
#include "Building.h"
#include "../Map/Map.h"
#include "../Systems/CommandSystem.h"
#include "../Systems/MovementSystem.h"
#include "../Graphics/Renderer.h"

// TODO: these numbers work good for fast game speed, but not very good for slow one
const float Unit::REPATH_INTERVAL = 0.3f; // recalculate path every 0.3 seconds
const float Unit::STUCK_TIMEOUT = 0.5f;   // consider stuck after 0.5s without moving
const float Unit::STUCK_FORCE_REPATH_TIMEOUT = 1.5f; // force repath after 1.5s stuck

Unit::Unit(int id, int ownerId, UnitType unitType)
    : Entity(id, EntityType::UNIT, ownerId)
    , unitType(unitType)
    , state(UnitState::IDLE)
    , currentPathIndex(0)
    , speed(2.0f)
    , attackDamage(5)
    , attackRange(1)
    , attackSpeed(1.0f)
    , timeSinceLastAttack(0)
    , attackTarget(nullptr)
    , armor(0)
    , visionRange(5)
    , carryingResource(ResourceType::NONE)
    , carriedAmount(0)
    , maxCarryAmount(10)
    , buildingTarget(nullptr)
    , buildProgress(0)
    , buildSpeed(0.1f)
    , captureTarget(nullptr)
    , repathTimer(0.0f)
    , mapRef(nullptr)
    , captureProgress(0)
    , movementSystem(nullptr)
    , stuckTimer(0.0f)
    , lastStuckCheckPosition(0, 0)
{
    width = 1;
    height = 1;
    
    switch (unitType) {
        case UnitType::PEASANT:
            maxHealth = 40;
            currentHealth = 40;
            speed = 2.0f;
            attackDamage = 3;
            attackRange = 1;
            armor = 0;
            break;
            
        case UnitType::WARRIOR:
            maxHealth = 60;
            currentHealth = 60;
            speed = 2.5f;
            attackDamage = 10;
            attackRange = 1;
            armor = 2;
            break;
            
        case UnitType::VETERAN:
            maxHealth = 80;
            currentHealth = 80;
            speed = 2.5f;
            attackDamage = 15;
            attackRange = 1;
            armor = 4;
            break;
            
        case UnitType::PALADIN:
            maxHealth = 120;
            currentHealth = 120;
            speed = 3.0f;
            attackDamage = 20;
            attackRange = 1;
            armor = 6;
            break;
    }
}

Unit::~Unit() {
}

void Unit::Update(float deltaTime) {
    if (!IsAlive()) {
        state = UnitState::DEAD;
        return;
    }
    
    switch (state) {
        case UnitState::MOVING:
            UpdateMovement(deltaTime);
            break;
            
        case UnitState::ATTACKING:
            UpdateCombat(deltaTime);
            break;
            
        case UnitState::MINING:
            UpdateMining(deltaTime);
            break;
            
        case UnitState::BUILDING:
            UpdateBuilding(deltaTime);
            break;
            
        case UnitState::CAPTURING:
            UpdateCapturing(deltaTime);
            break;
            
        case UnitState::REMOVING_OBSTACLE:
            UpdateObstacleRemoval(deltaTime);
            break;
            
        case UnitState::IDLE:
            ProcessNextCommand();
            break;
            
        default:
            break;
    }
}

void Unit::Render(Renderer* renderer) {
    Vector2 worldPos(position.x, position.y);
    
    // Load unit texture
    Texture* unitTex = renderer->LoadTexture("assets/textures/units/Human.png");
    
    if (unitTex) {
        // Render unit with texture - use full texture (500x500 is a single frame)
        glm::vec3 color(1.0f, 0.7f, 0.7f); // Slight tint for player
        if (ownerId == 1) color = glm::vec3(0.7f, 0.7f, 1.0f); // Blue tint for AI
        
        renderer->DrawTexturedRect(unitTex, worldPos, 32.0f, 32.0f, color,
                                   0.0f, 0.0f, 1.0f, 1.0f);
    } else {
        // Fallback to colored rect
        glm::vec3 color(1.0f, 0.0f, 0.0f);
        if (ownerId == 1) color = glm::vec3(0.0f, 0.0f, 1.0f);
        
        renderer->DrawRect(
            Rect(position.x, position.y, 32, 32),
            color
        );
    }
    
    // Draw selection indicator
    if (selected) {
        renderer->DrawCircle(
            Vector2(position.x + 16, position.y + 16),
            20,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
    
    // Draw health bar above the unit
    float healthPercent = (float)currentHealth / maxHealth;
    renderer->DrawRect(
        Rect(position.x, position.y + 32, 32 * healthPercent, 3),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

void Unit::GiveCommand(std::shared_ptr<Command> command, bool queue) {
    if (queue) {
        commandQueue.push(command);
    } else {
        ClearCommands();
        commandQueue.push(command);
        ProcessNextCommand();
    }
}

void Unit::ClearCommands() {
    while (!commandQueue.empty()) {
        commandQueue.pop();
    }
    currentCommand = nullptr;
    StopCurrentAction();
}

void Unit::StopCurrentAction() {
    state = UnitState::IDLE;
    path.clear();
    attackTarget = nullptr;
    buildingTarget = nullptr;
    captureTarget = nullptr;
    stuckTimer = 0.0f;
}

void Unit::MoveTo(const Point2D& destination) {
    // Path will be set by command system
    state = UnitState::MOVING;
    currentPathIndex = 0;
    stuckTimer = 0.0f;
    lastStuckCheckPosition = position;
}

void Unit::SetPath(const std::vector<Point2D>& newPath) {
    path = newPath;
    currentPathIndex = 0;
    if (!path.empty()) {
        state = UnitState::MOVING;
        stuckTimer = 0.0f;
        lastStuckCheckPosition = position;
    }
}

void Unit::Attack(Entity* target) {
    attackTarget = target;
    state = UnitState::ATTACKING;
}

void Unit::GatherResource(ResourceType resType, const Point2D& location) {
    carryingResource = resType;
    resourceLocation = location;
    state = UnitState::MINING;
}

void Unit::BuildStructure(Building* building) {
    buildingTarget = building;
    buildProgress = 0;
    state = UnitState::BUILDING;
}

void Unit::RemoveObstacle(Obstacle* obstacle) {
    attackTarget = obstacle;
    state = UnitState::REMOVING_OBSTACLE;
}

void Unit::CaptureBuilding(Building* building) {
    captureTarget = building;
    captureProgress = 0;
    state = UnitState::CAPTURING;
}

void Unit::UpgradeToWarrior() {
    if (unitType == UnitType::PEASANT) {
        unitType = UnitType::WARRIOR;
        maxHealth = 60;
        currentHealth = 60;
        speed = 2.5f;
        attackDamage = 10;
        armor = 2;
    }
}

void Unit::UpgradeToVeteran() {
    if (unitType == UnitType::WARRIOR) {
        unitType = UnitType::VETERAN;
        maxHealth = 80;
        currentHealth = 80;
        attackDamage = 15;
        armor = 4;
    }
}

void Unit::UpgradeToPaladin() {
    if (unitType == UnitType::VETERAN) {
        unitType = UnitType::PALADIN;
        maxHealth = 120;
        currentHealth = 120;
        speed = 3.0f;
        attackDamage = 20;
        armor = 6;
    }
}

void Unit::ApplyHealthUpgrade(int level) {
    maxHealth += level * 10;
    currentHealth += level * 10;
}

void Unit::ApplyToolsUpgrade(int level) {
    if (unitType == UnitType::PEASANT) {
        buildSpeed += level * 0.05f;
    }
}

void Unit::ApplyShieldsUpgrade(int level) {
    armor += level * 2;
}

void Unit::ApplySwordsUpgrade(int level) {
    attackDamage += level * 3;
}

void Unit::UpdateMovement(float deltaTime) {
    if (path.empty() || currentPathIndex >= path.size()) {
        state = UnitState::IDLE;
        return;
    }
    
    // Periodic repathing
    repathTimer += deltaTime;
    if (repathTimer >= REPATH_INTERVAL && mapRef && !path.empty()) {
        repathTimer = 0.0f;
        Point2D finalDest = path.back();
        Point2D currentGrid = GetGridPosition();
        
        // Check if path is still valid
        if (!Pathfinding::IsPathValid(path, mapRef, id)) {
            // Path is blocked, recalculate
            std::vector<Point2D> newPath = Pathfinding::FindPath(
                mapRef, currentGrid, finalDest, 1, id);
            if (!newPath.empty()) {
                path = newPath;
                currentPathIndex = 0;
            } else {
                // No path available
                state = UnitState::IDLE;
                path.clear();
                return;
            }
        }
    }
    
    // Stuck detection
    stuckTimer += deltaTime;
    float distanceMoved = position.Distance(lastStuckCheckPosition);
    
    if (distanceMoved > 2.0f) {
        stuckTimer = 0.0f;
        lastStuckCheckPosition = position;
    } else if (stuckTimer > STUCK_FORCE_REPATH_TIMEOUT) {
        // Force repath
        Point2D finalDest = path.back();
        Point2D currentGrid = GetGridPosition();
        std::vector<Point2D> newPath = Pathfinding::FindPath(
            mapRef, currentGrid, finalDest, 1, id);
        if (!newPath.empty()) {
            path = newPath;
            currentPathIndex = 0;
        }
        stuckTimer = 0.0f;
        lastStuckCheckPosition = position;
    }
    
    Point2D targetTile = path[currentPathIndex];
    Vector2 targetPos(targetTile.x * 32.0f + 16, targetTile.y * 32.0f + 16);
    
    Vector2 toTarget = targetPos - position;
    float distanceToTarget = toTarget.Length();
    
    // Reached waypoint
    if (distanceToTarget < 8.0f) {
        currentPathIndex++;
        if (currentPathIndex >= path.size()) {
            state = UnitState::IDLE;
            path.clear();
        }
        return;
    }
    
    // Calculate desired velocity
    Vector2 desiredDirection = toTarget.Normalized();
    Vector2 desiredVelocity = desiredDirection * (speed * 32.0f);
    
    // Apply simple collision avoidance
    Vector2 actualVelocity = ApplyCollisionAvoidance(desiredVelocity, deltaTime);
    
    // Move
    Vector2 newPosition = position + actualVelocity * deltaTime;
    
    // Validate and apply
    if (ValidatePosition(newPosition)) {
        position = newPosition;
    } else {
        // Try sliding
        Vector2 slideVelocity = TrySlideMovement(actualVelocity, deltaTime);
        if (slideVelocity.Length() > 0.1f) {
            Vector2 slidePosition = position + slideVelocity * deltaTime;
            if (ValidatePosition(slidePosition)) {
                position = slidePosition;
            }
        }
    }
    
    // Update spatial grid if available
    if (movementSystem) {
        Vector2 oldPos = position;
        movementSystem->UpdateUnitPosition(this, oldPos, position);
    }
}


void Unit::UpdateCombat(float deltaTime) {
    if (!attackTarget || !attackTarget->IsAlive()) {
        attackTarget = nullptr;
        state = UnitState::IDLE;
        return;
    }
    
    float distance = position.Distance(attackTarget->GetPosition());
    
    if (distance > attackRange * 32.0f) {
        // Move closer
        // Request pathfinding to target
        state = UnitState::IDLE;
        return;
    }
    
    timeSinceLastAttack += deltaTime;
    
    if (timeSinceLastAttack >= 1.0f / attackSpeed) {
        attackTarget->TakeDamage(attackDamage);
        timeSinceLastAttack = 0;
        
        if (!attackTarget->IsAlive()) {
            attackTarget = nullptr;
            state = UnitState::IDLE;
        }
    }
}

void Unit::UpdateMining(float deltaTime) {
    if (carryingResource == ResourceType::NONE) {
        // Go to resource location
        // Mine resource
        carriedAmount = maxCarryAmount;
    } else {
        // Return to storage
        // Deposit resources
        carryingResource = ResourceType::NONE;
        carriedAmount = 0;
    }
}

void Unit::UpdateBuilding(float deltaTime) {
    if (!buildingTarget) {
        state = UnitState::IDLE;
        return;
    }
    
    float distance = position.Distance(buildingTarget->GetPosition());
    
    if (distance > 50.0f) {
        // Too far, move closer
        state = UnitState::IDLE;
        return;
    }
    
    buildProgress += buildSpeed * deltaTime;
    buildingTarget->AddConstructionProgress(buildSpeed * deltaTime);
    
    if (buildingTarget->IsCompleted()) {
        buildingTarget = nullptr;
        state = UnitState::IDLE;
    }
}

void Unit::UpdateCapturing(float deltaTime) {
    if (!captureTarget || !captureTarget->IsAlive()) {
        captureTarget = nullptr;
        state = UnitState::IDLE;
        return;
    }
    
    captureProgress += deltaTime;
}

void Unit::UpdateObstacleRemoval(float deltaTime) {
    if (!attackTarget || !attackTarget->IsAlive()) {
        attackTarget = nullptr;
        state = UnitState::IDLE;
        return;
    }
    
    timeSinceLastAttack += deltaTime;
    
    if (timeSinceLastAttack >= 1.0f) {
        attackTarget->TakeDamage(10);
        timeSinceLastAttack = 0;
    }
}

void Unit::ProcessNextCommand() {
    if (commandQueue.empty()) {
        return;
    }
    
    currentCommand = commandQueue.front();
    commandQueue.pop();
    
    // Execute command (command will change unit state)
}

void Unit::ReturnResourcesToStorage() {
    // Find nearest storage building
    // Move to it
    // Deposit resources
}

void Unit::Serialize(SaveSystem* saveSystem) {
    Entity::Serialize(saveSystem);
    // Serialize unit-specific data
}

void Unit::Deserialize(SaveSystem* saveSystem) {
    Entity::Deserialize(saveSystem);
    // Deserialize unit-specific data
}

Vector2 Unit::ApplyCollisionAvoidance(const Vector2& desiredVelocity, float deltaTime) {
    if (!mapRef) return desiredVelocity;
    
    const float avoidanceRadius = 64.0f; // 2 tiles
    const float minSeparation = 20.0f; // 0.625 tiles
    
    Vector2 avoidanceForce(0, 0);
    int neighborCount = 0;
    
    // Get nearby units only
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        float distance = position.Distance(other->GetPosition());
        
        if (distance > 0 && distance < avoidanceRadius) {
            Vector2 diff = position - other->GetPosition();
            Vector2 pushDir = diff.Normalized();
            
            // Stronger force when very close
            float strength = 1.0f - (distance / avoidanceRadius);
            
            if (distance < minSeparation) {
                // Very close - strong repulsion
                strength = 2.0f * (minSeparation - distance) / minSeparation;
            }
            
            avoidanceForce = avoidanceForce + pushDir * strength;
            neighborCount++;
        }
    }
    
    if (neighborCount == 0) {
        return desiredVelocity;
    }
    
    // Normalize avoidance force
    avoidanceForce = avoidanceForce * (1.0f / neighborCount);
    
    // Blend with desired velocity
    const float avoidanceWeight = 0.5f;
    Vector2 blended = desiredVelocity + avoidanceForce * avoidanceWeight * 32.0f;
    
    // Maintain speed
    float desiredSpeed = desiredVelocity.Length();
    if (blended.Length() > 0.01f) {
        return blended.Normalized() * desiredSpeed;
    }
    
    return desiredVelocity;
}


bool Unit::ValidatePosition(const Vector2& pos) const {
    Point2D gridPos(pos.x / 32, pos.y / 32);
    
    if (!mapRef) return false;
    if (!mapRef->IsInBounds(gridPos.x, gridPos.y)) return false;
    if (!mapRef->IsWalkable(gridPos.x, gridPos.y)) return false;
    
    // Hard block on static entities
    if (mapRef->IsTileBlockedByStaticEntity(gridPos.x, gridPos.y, id)) return false;
    
    // Soft check for units - allow some overlap for smoother movement
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        float distance = pos.Distance(other->GetPosition());
        if (distance < 12.0f) { // 0.375 tiles - very tight
            return false;
        }
    }
    
    return true;
}


Vector2 Unit::TrySlideMovement(const Vector2& desiredVelocity, float deltaTime) {
    // Try X only
    Vector2 xOnly(desiredVelocity.x, 0);
    Vector2 testPos = position + xOnly * deltaTime;
    if (ValidatePosition(testPos)) {
        return xOnly;
    }
    
    // Try Y only
    Vector2 yOnly(0, desiredVelocity.y);
    testPos = position + yOnly * deltaTime;
    if (ValidatePosition(testPos)) {
        return yOnly;
    }
    
    // Try half X + full Y
    Vector2 halfX(desiredVelocity.x * 0.5f, desiredVelocity.y);
    testPos = position + halfX * deltaTime;
    if (ValidatePosition(testPos)) {
        return halfX;
    }
    
    // Try full X + half Y
    Vector2 halfY(desiredVelocity.x, desiredVelocity.y * 0.5f);
    testPos = position + halfY * deltaTime;
    if (ValidatePosition(testPos)) {
        return halfY;
    }
    
    return Vector2(0, 0);
}
