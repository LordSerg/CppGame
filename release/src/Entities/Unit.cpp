#include "Unit.h"
#include "Building.h"
#include "../Map/Map.h"
#include "../Systems/CommandSystem.h"
#include "../Graphics/Renderer.h"

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
    , captureProgress(0)
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
    Vector2 screenPos = renderer->WorldToScreen(position);
    
    // Render unit based on type and owner
    glm::vec3 color(1.0f, 0.0f, 0.0f); // Default red for player
    if (ownerId == 1) color = glm::vec3(0.0f, 0.0f, 1.0f); // Blue for AI
    
    renderer->DrawRect(
        Rect(screenPos.x, screenPos.y, 32, 32),
        color
    );
    
    // Draw selection indicator
    if (selected) {
        renderer->DrawCircle(
            Vector2(screenPos.x + 16, screenPos.y + 16),
            20,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
    
    // Draw health bar
    float healthPercent = (float)currentHealth / maxHealth;
    renderer->DrawRect(
        Rect(screenPos.x, screenPos.y - 5, 32 * healthPercent, 3),
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
}

void Unit::MoveTo(const Point2D& destination) {
    // Path will be set by command system
    state = UnitState::MOVING;
    currentPathIndex = 0;
}

void Unit::SetPath(const std::vector<Point2D>& newPath) {
    path = newPath;
    currentPathIndex = 0;
    if (!path.empty()) {
        state = UnitState::MOVING;
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
    
    Point2D targetTile = path[currentPathIndex];
    Vector2 targetPos(targetTile.x * 32.0f + 16, targetTile.y * 32.0f + 16);
    
    Vector2 direction = (targetPos - position).Normalized();
    float distance = position.Distance(targetPos);
    
    float moveDistance = speed * 32.0f * deltaTime;
    
    if (distance <= moveDistance) {
        position = targetPos;
        currentPathIndex++;
        
        if (currentPathIndex >= path.size()) {
            state = UnitState::IDLE;
            path.clear();
        }
    } else {
        position = position + direction * moveDistance;
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