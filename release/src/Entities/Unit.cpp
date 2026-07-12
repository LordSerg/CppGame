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
const float Unit::NEARBY_UPDATE_INTERVAL = 0.5f;

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
    , steeringSystem(nullptr)
    , velocity(0, 0)
    , smoothedVelocity(0, 0)
    , nearbyUpdateTimer(0.0f)
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
    // Position is now the CENTER of the unit
    // For a 32x32 sprite, we need to offset by -16 to draw from center
    Vector2 renderPos(position.x - 16.0f, position.y - 16.0f);
    
    // Load unit texture
    Texture* unitTex = renderer->LoadTexture("assets/textures/units/Human.png");
    
    if (unitTex) {
        glm::vec3 color(1.0f, 0.7f, 0.7f);
        if (ownerId == 1) color = glm::vec3(0.7f, 0.7f, 1.0f);
        
        renderer->DrawTexturedRect(unitTex, renderPos, 32.0f, 32.0f, color,
                                   0.0f, 0.0f, 1.0f, 1.0f);
    } else {
        glm::vec3 color(1.0f, 0.0f, 0.0f);
        if (ownerId == 1) color = glm::vec3(0.0f, 0.0f, 1.0f);
        
        renderer->DrawRect(
            Rect(renderPos.x, renderPos.y, 32, 32),
            color
        );
    }
    
    // Selection indicator - at unit center
    if (selected) {
        renderer->DrawCircle(
            position, // Already at center
            20,
            glm::vec3(0.0f, 1.0f, 0.0f)
        );
    }
    
    // Health bar - above unit
    float healthPercent = (float)currentHealth / maxHealth;
    float barWidth = 32.0f * healthPercent;
    renderer->DrawRect(
        Rect(renderPos.x, position.y - 16.0f - 5.0f, barWidth, 3),
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

void Unit::MoveToPosition(const Vector2& worldPosition) {
    if (!mapRef || !mapRef->GetNavMesh()) {
        state = UnitState::IDLE;
        return;
    }
    
    NavMesh* navMesh = mapRef->GetNavMesh();
    navPath = navMesh->FindPath(position, worldPosition);
    
    if (!navPath.IsComplete()) {
        state = UnitState::MOVING;
        velocity = Vector2(0, 0);
        smoothedVelocity = Vector2(0, 0);
    } else {
        state = UnitState::IDLE;
    }
}

void Unit::MoveToTile(const Point2D& tile) {
    Vector2 worldPos(tile.x * 32.0f + 16.0f, tile.y * 32.0f + 16.0f);
    MoveToPosition(worldPos);
}

void Unit::SetNavPath(const NavPath& path) {
    navPath = path;
    if (!navPath.IsComplete()) {
        state = UnitState::MOVING;
        velocity = Vector2(0, 0);
        smoothedVelocity = Vector2(0, 0);
    }
}

void Unit::MoveTo(const Point2D& destination) {
    MoveToTile(destination);
}

void Unit::SetPath(const std::vector<Point2D>& oldPath) {
    if (!oldPath.empty()) {
        MoveToTile(oldPath.back());
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
    if (navPath.IsComplete()) {
        state = UnitState::IDLE;
        velocity = Vector2(0, 0);
        smoothedVelocity = Vector2(0, 0);
        return;
    }
    
    if (!mapRef) return;
    
    // Update nearby units cache periodically
    nearbyUpdateTimer += deltaTime;
    if (nearbyUpdateTimer >= NEARBY_UPDATE_INTERVAL) {
        nearbyUpdateTimer = 0.0f;
        
        // Get all units from map
        std::vector<Entity*> allEntities = mapRef->GetAllEntities();
        std::vector<Unit*> allUnits;
        for (Entity* e : allEntities) {
            if (e && e->GetType() == EntityType::UNIT && e->IsAlive()) {
                allUnits.push_back(static_cast<Unit*>(e));
            }
        }
        
        UpdateNearbyUnits(allUnits);
    }
    
    // Calculate steering forces
    SteeringSystem* steering = mapRef->GetSteeringSystem();
    if (steering) {
        SteeringForces forces = steering->CalculateSteering(this, nearbyUnits, deltaTime);
        
        // Apply forces to velocity
        Vector2 totalForce = forces.GetTotal();
        Vector2 acceleration = totalForce * deltaTime;
        velocity = velocity + acceleration;
        
        // Limit speed
        float maxSpeed = speed * 32.0f;
        if (velocity.Length() > maxSpeed) {
            velocity = velocity.Normalized() * maxSpeed;
        }
        
        // Smooth velocity for natural movement
        smoothedVelocity.x = Math::Lerp(smoothedVelocity.x, velocity.x, deltaTime * 5.0f);
        smoothedVelocity.y = Math::Lerp(smoothedVelocity.y, velocity.y, deltaTime * 5.0f);
        
        // Update position
        Vector2 newPosition = position + smoothedVelocity * deltaTime;
        
        // Basic validation: make sure we're still on walkable navmesh
        NavMesh* navMesh = mapRef->GetNavMesh();
        if (navMesh && navMesh->GetNodeAt(newPosition) >= 0) {
            position = newPosition;
        } else {
            // Hit non-walkable area, slow down
            velocity = velocity * 0.5f;
        }
    } else {
        // Fallback: simple movement without steering
        Vector2 currentWaypoint = navPath.GetCurrentWaypoint();
        Vector2 toWaypoint = currentWaypoint - position;
        float distance = toWaypoint.Length();
        
        if (distance > 0.1f) {
            Vector2 direction = toWaypoint.Normalized();
            velocity = direction * (speed * 32.0f);
            position = position + velocity * deltaTime;
        }
    }
    
    // Check if we reached current waypoint
    Vector2 currentWaypoint = navPath.GetCurrentWaypoint();
    float distanceToWaypoint = position.Distance(currentWaypoint);
    
    if (distanceToWaypoint < 8.0f) {
        navPath.AdvanceWaypoint();
        
        if (navPath.IsComplete()) {
            state = UnitState::IDLE;
            velocity = Vector2(0, 0);
            smoothedVelocity = Vector2(0, 0);
        }
    }
    
    // Update spatial grid if available
    if (movementSystem) {
        Vector2 oldPos = position;
        movementSystem->UpdateUnitPosition(this, oldPos, position);
    }
}

void Unit::CalculatePathTo(const Vector2& destination) {
    if (!mapRef) return;
    
    navPath = mapRef->GetNavMesh()->FindPath(position, destination);
}

void Unit::UpdateNearbyUnits(const std::vector<Unit*>& allEntities) {
    nearbyUnits.clear();
    
    Point2D myGrid = GetGridPosition();
    
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        Unit* otherUnit = static_cast<Unit*>(other);
        
        // Quick distance check
        float distance = position.Distance(otherUnit->GetPosition());
        if (distance > 200.0f) continue; // Skip if too far
        
        nearbyUnits.push_back(otherUnit);
    }
}

void Unit::UpdateVelocity(float deltaTime, const Vector2& targetVelocity) {
    // Smooth velocity changes
    velocity.x = Math::Lerp(velocity.x, targetVelocity.x, deltaTime * 8.0f);
    velocity.y = Math::Lerp(velocity.y, targetVelocity.y, deltaTime * 8.0f);
}



bool Unit::TrySimpleSlide(const Vector2& desiredVelocity, float deltaTime) {
    Vector2 xOnly(desiredVelocity.x, 0);
    Vector2 testPos = position + xOnly * deltaTime;
    if (ValidatePosition(testPos)) {
        position = testPos;
        return true;
    }
    
    Vector2 yOnly(0, desiredVelocity.y);
    testPos = position + yOnly * deltaTime;
    if (ValidatePosition(testPos)) {
        position = testPos;
        return true;
    }
    
    return false;
}

void Unit::HandleCollision(Unit* otherUnit, float deltaTime) {
    if (!otherUnit) return;
    
    // Determine who has priority based on ID (lower ID = higher priority)
    bool iHavePriority = (id < otherUnit->GetId());
    
    if (iHavePriority) {
        // I have priority - the other unit should step aside
        // I just wait briefly for them to move
        collisionWaitTimer += deltaTime;
        
        if (collisionWaitTimer > 1.0f) {
            // Waited too long, try to go around
            Vector2 toOther = otherUnit->GetPosition() - position;
            Vector2 perpendicular(-toOther.y, toOther.x);
            perpendicular = perpendicular.Normalized() * 16.0f;
            
            Vector2 sideStep = position + perpendicular * deltaTime;
            if (ValidatePosition(sideStep)) {
                position = sideStep;
            }
            
            collisionWaitTimer = 0.0f;
        }
    } else {
        // Other unit has priority - I need to step aside
        collidingWithUnitId = otherUnit->GetId();
        shouldStepAside = true;
        
        // Calculate perpendicular direction to step aside
        Vector2 toOther = otherUnit->GetPosition() - position;
        
        // Step perpendicular to the line between us
        Vector2 perpendicular(-toOther.y, toOther.x);
        
        // Choose direction based on path
        if (!path.empty()) {
            Point2D finalDest = path.back();
            Vector2 toGoal(finalDest.x * 32.0f + 16.0f - position.x,
                          finalDest.y * 32.0f + 16.0f - position.y);
            
            // Pick the perpendicular direction more aligned with our goal
            float dot1 = perpendicular.x * toGoal.x + perpendicular.y * toGoal.y;
            if (dot1 < 0) {
                perpendicular = Vector2(toOther.y, -toOther.x);
            }
        }
        
        perpendicular = perpendicular.Normalized() * (speed * 32.0f);
        Vector2 stepAsidePos = position + perpendicular * deltaTime;
        
        if (ValidatePosition(stepAsidePos)) {
            position = stepAsidePos;
        } else {
            // Try opposite direction
            perpendicular = Vector2(-perpendicular.x, -perpendicular.y);
            stepAsidePos = position + perpendicular * deltaTime;
            if (ValidatePosition(stepAsidePos)) {
                position = stepAsidePos;
            }
        }
        
        // After stepping aside, check if we're clear
        float distToOther = position.Distance(otherUnit->GetPosition());
        if (distToOther > 40.0f) {
            // Far enough away, resume normal movement
            shouldStepAside = false;
            collidingWithUnitId = -1;
        }
    }
}

Unit* Unit::FindBlockingUnit(const Vector2& targetPosition) const {
    if (!mapRef) return nullptr;
    
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        float distance = targetPosition.Distance(other->GetPosition());
        if (distance < 20.0f) {
            return static_cast<Unit*>(other);
        }
    }
    
    return nullptr;
}

bool Unit::IsCollidingWith(Unit* otherUnit) const {
    if (!otherUnit) return false;
    return collidingWithUnitId == otherUnit->GetId();
}

Vector2 Unit::ApplySmartAvoidance(const Vector2& desiredVelocity, float deltaTime) {
    if (!mapRef) return desiredVelocity;
    
    const float detectionRadius = 48.0f; // Look ahead distance
    const float avoidanceRadius = 32.0f; // Personal space
    
    Vector2 avoidanceForce(0, 0);
    Vector2 steeringForce(0, 0);
    int obstacleCount = 0;
    
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    
    Vector2 desiredDir = desiredVelocity.Normalized();
    
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        Vector2 toOther = other->GetPosition() - position;
        float distance = toOther.Length();
        
        if (distance < 0.1f || distance > detectionRadius) continue;
        
        Vector2 toOtherDir = toOther.Normalized();
        
        // Check if this unit is in our path (dot product > 0 means ahead of us)
        float dotProduct = desiredDir.x * toOtherDir.x + desiredDir.y * toOtherDir.y;
        
        if (distance < avoidanceRadius) {
            // Too close - always apply repulsion
            Vector2 pushDir = (position - other->GetPosition()).Normalized();
            float strength = 1.5f * (1.0f - distance / avoidanceRadius);
            avoidanceForce = avoidanceForce + pushDir * strength;
            obstacleCount++;
            
            // Add steering force to go AROUND the obstacle, not just away from it
            // Perpendicular vector to the obstacle
            Vector2 perpendicular(-pushDir.y, pushDir.x);

            // KEY FIX: Deterministic tie-breaker for head-on collisions
            // Ensure two units facing each other pick opposite sides to pass
            if (id > other->GetId()) {
                perpendicular = Vector2(pushDir.y, -pushDir.x);
            }
            
            // Choose direction based on which side is more open
            Vector2 leftOption = position + perpendicular * 20.0f;
            Vector2 rightOption = position - perpendicular * 20.0f;
            
            bool leftClear = ValidatePosition(leftOption);
            bool rightClear = ValidatePosition(rightOption);
            
            if (leftClear && !rightClear) {
                steeringForce = steeringForce + perpendicular * 0.5f;
            } else if (rightClear && !leftClear) {
                steeringForce = steeringForce - perpendicular * 0.5f;
            } else if (leftClear && rightClear) {
                // Both sides clear, pick based on path direction
                // Calculate which perpendicular direction is more aligned with our goal
                Point2D finalDest = path.empty() ? GetGridPosition() : path.back();
                Vector2 toGoal = Vector2(finalDest.x * 32.0f + 16.0f, 
                                        finalDest.y * 32.0f + 16.0f) - position;
                Vector2 toGoalDir = toGoal.Normalized();
                
                float leftDot = perpendicular.x * toGoalDir.x + perpendicular.y * toGoalDir.y;
                float rightDot = -perpendicular.x * toGoalDir.x + -perpendicular.y * toGoalDir.y;
                
                if (leftDot > rightDot) {
                    steeringForce = steeringForce + perpendicular * 0.5f;
                } else {
                    steeringForce = steeringForce - perpendicular * 0.5f;
                }
            }
            
        } else if (dotProduct > 0.3f && distance < detectionRadius) {
            // Unit is ahead of us in our path - apply predictive avoidance
            Unit* otherUnit = static_cast<Unit*>(other);
            
            // Predict where we'll be
            Vector2 myFuturePos = position + desiredVelocity * 0.5f;
            Vector2 otherFuturePos = other->GetPosition();
            
            // If other unit is moving, predict its position
            if (otherUnit->IsMoving() && !otherUnit->GetPath().empty()) {
                Vector2 otherVelocity = (other->GetPosition() - position); // Approximate
                otherFuturePos = other->GetPosition() + otherVelocity.Normalized() * 
                                otherUnit->GetSpeed() * 16.0f;
            }
            
            float futureDistance = myFuturePos.Distance(otherFuturePos);
            
            if (futureDistance < avoidanceRadius) {
                // Will collide - start avoiding now
                Vector2 avoidDir = (position - other->GetPosition()).Normalized();
                float strength = 0.8f * (1.0f - futureDistance / avoidanceRadius);
                avoidanceForce = avoidanceForce + avoidDir * strength;
                
                // Also add steering
                Vector2 perpendicular(-avoidDir.y, avoidDir.x);
                steeringForce = steeringForce + perpendicular * 0.3f;
                obstacleCount++;
            }
        }
    }
    
    if (obstacleCount == 0) {
        return desiredVelocity;
    }
    
    // Combine forces
    avoidanceForce = avoidanceForce * (1.0f / obstacleCount);
    steeringForce = steeringForce * (1.0f / obstacleCount);
    
    // Apply forces with appropriate weights
    Vector2 combinedForce = desiredVelocity + 
                           avoidanceForce * 60.0f + 
                           steeringForce * 40.0f;
    
    // Maintain speed
    float desiredSpeed = desiredVelocity.Length();
    float currentSpeed = combinedForce.Length();
    
    if (currentSpeed > 0.01f) {
        // Allow some speed reduction when avoiding heavily
        float targetSpeed = std::max(desiredSpeed * 0.4f, 
                                    std::min(desiredSpeed, currentSpeed));
        return combinedForce.Normalized() * targetSpeed;
    }
    
    return desiredVelocity;
}

bool Unit::TryAlternativeMovement(const Vector2& desiredVelocity, float deltaTime) {
    // Try various alternative directions to unstick the unit
    
    // 1. Try pure X movement
    Vector2 xOnly(desiredVelocity.x, 0);
    Vector2 testPos = position + xOnly * deltaTime;
    if (ValidatePosition(testPos)) {
        position = testPos;
        return true;
    }
    
    // 2. Try pure Y movement
    Vector2 yOnly(0, desiredVelocity.y);
    testPos = position + yOnly * deltaTime;
    if (ValidatePosition(testPos)) {
        position = testPos;
        return true;
    }
    
    // 3. Try diagonal alternatives (favor X or Y based on desired direction)
    float absX = abs(desiredVelocity.x);
    float absY = abs(desiredVelocity.y);
    
    if (absX > absY) {
        // Favor X direction
        Vector2 xBias(desiredVelocity.x * 0.8f, desiredVelocity.y * 0.4f);
        testPos = position + xBias * deltaTime;
        if (ValidatePosition(testPos)) {
            position = testPos;
            return true;
        }
    } else {
        // Favor Y direction
        Vector2 yBias(desiredVelocity.x * 0.4f, desiredVelocity.y * 0.8f);
        testPos = position + yBias * deltaTime;
        if (ValidatePosition(testPos)) {
            position = testPos;
            return true;
        }
    }
    
    // 4. Try perpendicular movements (sideways slide)
    Vector2 perpLeft(-desiredVelocity.y * 0.5f, desiredVelocity.x * 0.5f);
    testPos = position + perpLeft * deltaTime;
    if (ValidatePosition(testPos)) {
        position = testPos;
        return true;
    }
    
    Vector2 perpRight(desiredVelocity.y * 0.5f, -desiredVelocity.x * 0.5f);
    testPos = position + perpRight * deltaTime;
    if (ValidatePosition(testPos)) {
        position = testPos;
        return true;
    }
    
    // 5. Try very small movement in desired direction
    Vector2 smallMove = desiredVelocity.Normalized() * 4.0f * deltaTime;
    testPos = position + smallMove;
    if (ValidatePosition(testPos)) {
        position = testPos;
        return true;
    }
    
    return false; // Couldn't move at all
}

bool Unit::IsTileOccupiedByOtherUnit(const Point2D& tile) const {
    if (!mapRef) return false;
    
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        Point2D otherTile = other->GetGridPosition();
        if (otherTile == tile) {
            return true;
        }
    }
    
    return false;
}

Unit* Unit::GetUnitAtTile(const Point2D& tile) const {
    if (!mapRef) return nullptr;
    
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        Point2D otherTile = other->GetGridPosition();
        if (otherTile == tile) {
            return static_cast<Unit*>(other);
        }
    }
    
    return nullptr;
}

Vector2 Unit::ApplyCollisionAvoidance(const Vector2& desiredVelocity, float deltaTime) {
    if (!mapRef) return desiredVelocity;
    
    const float avoidanceRadius = 48.0f;
    
    Vector2 avoidanceForce(0, 0);
    int neighborCount = 0;
    
    Point2D myGrid = GetGridPosition();
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        // Quick grid check
        Point2D otherGrid = other->GetGridPosition();
        int gridDist = abs(myGrid.x - otherGrid.x) + abs(myGrid.y - otherGrid.y);
        if (gridDist > 2) continue;
        
        float distance = position.Distance(other->GetPosition());
        
        if (distance > 0 && distance < avoidanceRadius) {
            Vector2 diff = position - other->GetPosition();
            Vector2 pushDir = diff.Normalized();
            
            // Simple repulsion based on distance
            float strength = 1.0f - (distance / avoidanceRadius);
            
            avoidanceForce = avoidanceForce + pushDir * strength;
            neighborCount++;
        }
    }
    
    if (neighborCount == 0) {
        return desiredVelocity;
    }
    
    avoidanceForce = avoidanceForce * (1.0f / neighborCount);
    Vector2 blended = desiredVelocity + avoidanceForce * 40.0f;
    
    float desiredSpeed = desiredVelocity.Length();
    if (blended.Length() > 0.01f) {
        return blended.Normalized() * desiredSpeed;
    }
    
    return desiredVelocity;
}

bool Unit::ValidatePosition(const Vector2& pos) const {
    Point2D gridPos((int)(pos.x / 32.0f), (int)(pos.y / 32.0f));
    
    if (!mapRef) return false;
    if (!mapRef->IsInBounds(gridPos.x, gridPos.y)) return false;
    if (!mapRef->IsWalkable(gridPos.x, gridPos.y)) return false;
    if (mapRef->IsTileBlockedByStaticEntity(gridPos.x, gridPos.y, id)) return false;
    
    // Check diagonal corner cutting
    Point2D currentGrid = GetGridPosition();
    if (currentGrid != gridPos) {
        int dx = gridPos.x - currentGrid.x;
        int dy = gridPos.y - currentGrid.y;
        
        if (dx != 0 && dy != 0) {
            Point2D corner1(currentGrid.x + dx, currentGrid.y);
            Point2D corner2(currentGrid.x, currentGrid.y + dy);
            
            bool corner1Blocked = !mapRef->IsInBounds(corner1.x, corner1.y) ||
                                !mapRef->IsWalkable(corner1.x, corner1.y) ||
                                mapRef->IsTileBlockedByStaticEntity(corner1.x, corner1.y, id);
            
            bool corner2Blocked = !mapRef->IsInBounds(corner2.x, corner2.y) ||
                                !mapRef->IsWalkable(corner2.x, corner2.y) ||
                                mapRef->IsTileBlockedByStaticEntity(corner2.x, corner2.y, id);
            
            if (corner1Blocked && corner2Blocked) {
                return false;
            }
        }
    }
    
    // Check edge samples
    const float checkRadius = 14.0f;
    const int checkPoints = 8;
    
    for (int i = 0; i < checkPoints; i++) {
        float angle = (i / (float)checkPoints) * 2.0f * 3.14159f;
        float checkX = pos.x + cos(angle) * checkRadius;
        float checkY = pos.y + sin(angle) * checkRadius;
        
        Point2D checkGrid((int)(checkX / 32.0f), (int)(checkY / 32.0f));
        
        if (!mapRef->IsInBounds(checkGrid.x, checkGrid.y)) return false;
        if (!mapRef->IsWalkable(checkGrid.x, checkGrid.y)) return false;
        if (mapRef->IsTileBlockedByStaticEntity(checkGrid.x, checkGrid.y, id)) return false;
    }
    
    // Unit collision - allow if increasing separation
    std::vector<Entity*> allEntities = mapRef->GetAllEntities();
    for (Entity* other : allEntities) {
        if (!other || !other->IsAlive()) continue;
        if (other->GetId() == id) continue;
        if (other->GetType() != EntityType::UNIT) continue;
        
        float newDist = pos.Distance(other->GetPosition());
        float currentDist = position.Distance(other->GetPosition());
        
        // Allow movement if it increases distance OR if we're stepping aside
        if (newDist < 20.0f && newDist <= currentDist && !shouldStepAside) {
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
    
    // Try 75% X + 25% Y
    Vector2 biasX(desiredVelocity.x * 0.75f, desiredVelocity.y * 0.25f);
    testPos = position + biasX * deltaTime;
    if (ValidatePosition(testPos)) {
        return biasX;
    }
    
    // Try 25% X + 75% Y
    Vector2 biasY(desiredVelocity.x * 0.25f, desiredVelocity.y * 0.75f);
    testPos = position + biasY * deltaTime;
    if (ValidatePosition(testPos)) {
        return biasY;
    }
    
    return Vector2(0, 0);
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




