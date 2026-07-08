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
    
    // Periodic repathing to adapt to moving entities
    repathTimer += deltaTime;
    if (repathTimer >= REPATH_INTERVAL && mapRef && !path.empty()) {
        repathTimer = 0.0f;
        Point2D finalDest = path.back();
        Point2D currentGrid = GetGridPosition();
        // Only repath if we're not at the final destination
        if (currentGrid != finalDest) {
            std::vector<Point2D> newPath = Pathfinding::FindPath(mapRef, currentGrid, finalDest, 1, id);
            if (!newPath.empty() && newPath.size() > 1) {
                path = newPath;
                // Skip the first waypoint if it's the tile we're already on,
                // so the unit doesn't "go back" when between tiles
                currentPathIndex = (path[0] == currentGrid) ? 1 : 0;
            }
        }
    }
    
    // Stuck detection
    ApplyStuckRecovery(deltaTime);
    
    Point2D targetTile = path[currentPathIndex];
    Vector2 targetPos(targetTile.x * 32.0f, targetTile.y * 32.0f);
    
    Vector2 toTarget = targetPos - position;
    float distanceToTarget = toTarget.Length();
    
    // Check if we've reached the current waypoint
    if (distanceToTarget < 4.0f) {
        position = targetPos;
        currentPathIndex++;
        stuckTimer = 0.0f;
        lastStuckCheckPosition = position;
        
        if (currentPathIndex >= path.size()) {
            state = UnitState::IDLE;
            path.clear();
        }
        return;
    }
    
    // Save old position for spatial grid update
    Vector2 oldPosition = position;
    
    // Calculate desired velocity
    Vector2 desiredDirection = toTarget.Normalized();
    Vector2 desiredVelocity = desiredDirection * (speed * 32.0f);
    
    // Apply collision avoidance with nearby entities
    Vector2 actualVelocity = ApplyCollisionAvoidance(desiredVelocity, deltaTime);
    
    // Calculate new position
    Vector2 newPosition = position + actualVelocity * deltaTime;
    
    // Validate new position
    if (ValidatePosition(newPosition)) {
        position = newPosition;
    } else {
        // Try to slide along obstacle - attempt multiple slide directions
        Vector2 slideVelocity = TrySlideMovement(desiredVelocity, deltaTime);
        Vector2 slidePosition = position + slideVelocity * deltaTime;
        
        if (ValidatePosition(slidePosition)) {
            position = slidePosition;
        } else if (!TryRepath()) {
            // Can't move and can't re-path, skip this waypoint
            currentPathIndex++;
            stuckTimer = 0.0f;
            lastStuckCheckPosition = position;
        }
    }
    
    // Update spatial grid if we have a movement system reference
    if (movementSystem) {
        movementSystem->UpdateUnitPosition(this, oldPosition, position);
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
    
    const float avoidanceRadius = 48.0f; // 1.5 tiles
    const float separationWeight = 3.0f;
    const float wallAvoidanceWeight = 5.0f;
    // New: lateral force for passing other units head-on
    const float lateralForceWeight = 4.0f;
    
    Vector2 separationForce(0, 0);
    Vector2 lateralForce(0, 0);
    int neighborCount = 0;
    
    // Get all entities from the map and check nearby ones
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    
    // The desired direction (normalized) of where we want to go
    Vector2 desiredDir = Vector2(0, 0);
    if (desiredVelocity.Length() > 0.01f) {
        desiredDir = desiredVelocity.Normalized();
    }
    
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        
        Vector2 diff = position - other->GetPosition();
        float distance = diff.Length();
        
        if (distance > 0 && distance < avoidanceRadius) {
            // Stronger repulsion when closer
            float strength = 1.0f - (distance / avoidanceRadius);
            strength = strength * strength; // Quadratic falloff for smoother behavior
            
            // Base separation pushes directly away
            Vector2 pushDir = diff.Normalized();
            separationForce = separationForce + pushDir * strength;
            
            // KEY FIX: Add lateral/sideways force so units can pass each other
            // When two units face each other, separation pushes them both backwards (equal and opposite),
            // cancelling out. We also need to push sideways so they can pass.
            if (distance < avoidanceRadius * 0.7f) {
                // Create a perpendicular vector to the push direction (sideways)
                // This gives a rotationally-biased lateral offset
                Vector2 perpendicular(-pushDir.y, pushDir.x);
                
                // Dot product tells us if we're heading towards this entity
                float headingDot = desiredDir.x * (-pushDir.x) + desiredDir.y * (-pushDir.y);
                
                if (headingDot > 0.3f) {
                    // We're heading towards this entity - apply lateral force
                    // Alternate sides based on entity ID for deterministic behavior
                    float sideSign = (other->GetId() % 2 == 0) ? 1.0f : -1.0f;
                    
                    // Scale lateral force by how much we're heading towards them
                    float lateralStrength = strength * headingDot;
                    lateralForce = lateralForce + perpendicular * lateralStrength * sideSign;
                    neighborCount++;
                }
            }
            
            neighborCount++;
        }
    }
    
    // Separately handle buildings (static obstacles) with stronger force but no lateral
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::BUILDING) continue;
        
        Vector2 diff = position - other->GetPosition();
        float distance = diff.Length();
        
        if (distance > 0 && distance < avoidanceRadius * 1.5f) {
            float strength = 1.0f - (distance / (avoidanceRadius * 1.5f));
            strength = strength * strength;
            separationForce = separationForce + diff.Normalized() * strength * wallAvoidanceWeight;
            neighborCount++;
        }
    }
    
    if (neighborCount > 0) {
        separationForce = separationForce * (1.0f / neighborCount);
        if (lateralForce.Length() > 0.01f) {
            lateralForce = lateralForce * (1.0f / neighborCount);
        }
        
        // Combine forces: separation pushes away, lateral pushes sideways to pass
        Vector2 totalForce = separationForce * separationWeight * 32.0f;
        totalForce = totalForce + lateralForce * lateralForceWeight * 32.0f;
        
        Vector2 avoidanceVelocity = desiredVelocity + totalForce;
        
        // Maintain speed but allow slight speed reduction when maneuvering
        float desiredSpeed = desiredVelocity.Length();
        float currentSpeed = avoidanceVelocity.Length();
        if (currentSpeed > 0.01f) {
            if (currentSpeed > desiredSpeed) {
                avoidanceVelocity = avoidanceVelocity.Normalized() * desiredSpeed;
            }
            // Allow up to 40% speed reduction when navigating tight spaces
            if (currentSpeed < desiredSpeed * 0.4f) {
                avoidanceVelocity = avoidanceVelocity.Normalized() * (desiredSpeed * 0.4f);
            }
        }
        
        return avoidanceVelocity;
    }
    
    return desiredVelocity;
}

bool Unit::ValidatePosition(const Vector2& pos) const {
    Point2D gridPos(pos.x / 32, pos.y / 32);
    
    // Map bounds check
    if (!mapRef) {
        if (gridPos.x < 0 || gridPos.y < 0) return false;
        return true;
    }
    
    if (!mapRef->IsInBounds(gridPos.x, gridPos.y)) return false;
    
    // Check if the tile is walkable (terrain + trees/rocks)
    if (!mapRef->IsWalkable(gridPos.x, gridPos.y)) return false;
    
    // Check static barriers (buildings, obstacles) - these are always hard-blocked
    if (mapRef->IsTileBlockedByStaticEntity(gridPos.x, gridPos.y, id)) return false;
    
    // KEY FIX: Relaxed unit-to-unit collision to prevent deadlock
    // Allow units to pass each other more easily by checking the specific tile grid,
    // not the exact pixel position.
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        float distance = pos.Distance(other->GetPosition());
        // Reduced minimum separation: 16 pixels (0.5 tiles) was 22 (0.7 tiles)
        // This allows units to squeeze past each other in corridors
        if (distance < 16.0f) {
            return false;
        }
    }
    
    return true;
}

Vector2 Unit::TrySlideMovement(const Vector2& desiredVelocity, float deltaTime) {
    // Try moving along X axis only
    Vector2 xOnly(desiredVelocity.x, 0);
    Vector2 testPos = position + xOnly * deltaTime;
    
    if (ValidatePosition(testPos)) {
        return xOnly;
    }
    
    // Try moving along Y axis only
    Vector2 yOnly(0, desiredVelocity.y);
    testPos = position + yOnly * deltaTime;
    
    if (ValidatePosition(testPos)) {
        return yOnly;
    }
    
    // KEY FIX: Try diagonal movements for better obstacle navigation
    // Try moving along X * 0.5 + Y (biased slide)
    Vector2 diag1(desiredVelocity.x * 0.5f, desiredVelocity.y);
    testPos = position + diag1 * deltaTime;
    if (ValidatePosition(testPos)) {
        return diag1;
    }
    
    // Try moving along X + Y * 0.5 (biased slide)
    Vector2 diag2(desiredVelocity.x, desiredVelocity.y * 0.5f);
    testPos = position + diag2 * deltaTime;
    if (ValidatePosition(testPos)) {
        return diag2;
    }
    
    // Try moving along perpendicular (lateral slide)
    Vector2 perp(-desiredVelocity.y * 0.4f, desiredVelocity.x * 0.4f);
    testPos = position + perp * deltaTime;
    if (ValidatePosition(testPos)) {
        return perp;
    }
    
    // Try opposite perpendicular
    Vector2 perp2(desiredVelocity.y * 0.4f, -desiredVelocity.x * 0.4f);
    testPos = position + perp2 * deltaTime;
    if (ValidatePosition(testPos)) {
        return perp2;
    }
    
    return Vector2(0, 0);
}

bool Unit::TryRepath() {
    if (!mapRef || path.empty()) return false;
    
    Point2D finalDest = path.back();
    Point2D currentGrid = GetGridPosition();
    
    if (currentGrid == finalDest) return false;
    
    std::vector<Point2D> newPath = Pathfinding::FindPath(mapRef, currentGrid, finalDest, 1, id);
    if (!newPath.empty() && newPath.size() > 1) {
        path = newPath;
        // Skip the first waypoint if it's the tile we're already on
        currentPathIndex = (path[0] == currentGrid) ? 1 : 0;
        return true;
    }
    
    return false;
}

void Unit::ApplyStuckRecovery(float deltaTime) {
    // Track how far we've moved since last check
    float distanceMoved = position.Distance(lastStuckCheckPosition);
    
    if (distanceMoved < 1.0f) {
        // Barely moved - increment stuck timer
        stuckTimer += deltaTime;
    } else {
        // We're moving, reset stuck timer
        stuckTimer = 0.0f;
        lastStuckCheckPosition = position;
    }
    
    if (stuckTimer > STUCK_FORCE_REPATH_TIMEOUT) {
        // Been stuck for a while - force a path recalculation
        // This handles cases where the path is blocked by units that have moved
        if (mapRef && !path.empty()) {
            Point2D finalDest = path.back();
            Point2D currentGrid = GetGridPosition();
            if (currentGrid != finalDest) {
                std::vector<Point2D> newPath = Pathfinding::FindPath(mapRef, currentGrid, finalDest, 1, id);
                if (!newPath.empty() && newPath.size() > 1) {
                    path = newPath;
                    currentPathIndex = (path[0] == currentGrid) ? 1 : 0;
                }
            }
        }
        stuckTimer = 0.0f;
        lastStuckCheckPosition = position;
    }
}