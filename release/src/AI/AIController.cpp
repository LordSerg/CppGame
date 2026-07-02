#include "AIState.h"
#include "AIController.h"
#include "../Map/Map.h"
#include "../Map/Pathfinding.h"
#include "../Entities/Unit.h"
#include "../Entities/Peasant.h"
#include "../Entities/Fighter.h"
#include "../Entities/Building.h"
#include "../Systems/CommandSystem.h"
#include <algorithm>
#include <random>

AIController::AIController(int playerId, AIDifficulty difficulty)
    : playerId(playerId)
    , difficulty(difficulty)
    , currentPhase(AIGamePhase::EARLY_GAME)
    , timeSinceLastDecision(0)
{
    buildOrder = std::make_unique<BuildOrder>();
    aiState = std::make_unique<AIState>();  // This creates the class instance
    
    // Set decision interval based on difficulty
    switch (difficulty) {
        case AIDifficulty::EASY:
            decisionInterval = 2.0f;
            break;
        case AIDifficulty::MEDIUM:
            decisionInterval = 1.0f;
            break;
        case AIDifficulty::HARD:
            decisionInterval = 0.5f;
            break;
    }
}

AIController::~AIController() {
}

void AIController::Update(float deltaTime, Map* map, 
                         ResourceManager* resourceMgr,
                         TechTree* techTree,
                         CommandSystem* commandSystem) {
    timeSinceLastDecision += deltaTime;
    
    if (aiState) {
        aiState->Update(deltaTime);
    }
    
    if (timeSinceLastDecision >= decisionInterval) {
        MakeDecisions(map, resourceMgr, techTree, commandSystem);
        timeSinceLastDecision = 0;
    }
}

void AIController::MakeDecisions(Map* map, ResourceManager* resourceMgr,
                                TechTree* techTree, CommandSystem* commandSystem) {
    // Update AI state
    UpdateAIState(map);
    
    // Determine current game phase
    DetermineGamePhase(map, resourceMgr);
    
    // Make strategic decisions based on game phase
    ManageEconomy(map, resourceMgr, techTree, commandSystem);
    ManageTechTree(map, resourceMgr, techTree);
    ManageMilitary(map, resourceMgr, techTree, commandSystem);
    
    // Tactical decisions
    ScoutMap(map, commandSystem);
    
    if (ShouldAttack(map)) {
        AttackEnemy(map, commandSystem);
    } else if (IsUnderAttack(map)) {
        DefendBase(map, commandSystem);
    }
    
    BuildDefenses(map, resourceMgr, techTree, commandSystem);
}

void AIController::UpdateAIState(Map* map) {
    std::vector<Unit*> myUnits = GetMyUnits(map);
    
    int peasants = 0;
    int military = 0;
    int idleWorkers = 0;
    int idleMilitary = 0;
    
    for (Unit* unit : myUnits) {
        if (unit->GetUnitType() == UnitType::PEASANT) {
            peasants++;
            if (unit->GetState() == UnitState::IDLE) {
                idleWorkers++;
            }
        } else {
            military++;
            if (unit->GetState() == UnitState::IDLE) {
                idleMilitary++;
            }
        }
    }
    
    aiState->SetPeasantCount(peasants);
    aiState->SetMilitaryUnitCount(military);
    aiState->SetIdleWorkerCount(idleWorkers);
    aiState->SetIdleMilitaryCount(idleMilitary);
    
    // Update enemy information
    std::vector<Unit*> enemyUnits = GetEnemyUnits(map);
    std::vector<Building*> enemyBuildings = GetEnemyBuildings(map);
    
    if (!enemyUnits.empty() || !enemyBuildings.empty()) {
        Point2D avgPos(0, 0);
        int count = 0;
        
        for (Unit* unit : enemyUnits) {
            Point2D pos = unit->GetGridPosition();
            avgPos.x += pos.x;
            avgPos.y += pos.y;
            count++;
        }
        
        for (Building* building : enemyBuildings) {
            Point2D pos = building->GetGridPosition();
            avgPos.x += pos.x;
            avgPos.y += pos.y;
            count++;
        }
        
        if (count > 0) {
            avgPos.x /= count;
            avgPos.y /= count;
            
            aiState->UpdateEnemyInfo(0, avgPos, enemyUnits.size(), 
                                    enemyBuildings.size());
        }
    }
}

void AIController::DetermineGamePhase(Map* map, ResourceManager* resourceMgr) {
    const PlayerResources& res = resourceMgr->GetPlayerResources(playerId);
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    
    int totalResources = res.wood + res.metal + res.food;
    int buildingCount = myBuildings.size();
    
    // Simple phase determination
    if (totalResources < 500 || buildingCount < 5) {
        if (currentPhase != AIGamePhase::EARLY_GAME) {
            currentPhase = AIGamePhase::EARLY_GAME;
            buildOrder->SetEarlyGameOrder();
        }
    } else if (totalResources < 1500 || buildingCount < 10) {
        if (currentPhase != AIGamePhase::MID_GAME) {
            currentPhase = AIGamePhase::MID_GAME;
            buildOrder->SetMidGameOrder();
        }
    } else {
        if (currentPhase != AIGamePhase::LATE_GAME) {
            currentPhase = AIGamePhase::LATE_GAME;
            buildOrder->SetLateGameOrder();
        }
    }
}

void AIController::ManageEconomy(Map* map, ResourceManager* resourceMgr,
                                TechTree* techTree, CommandSystem* commandSystem) {
    const PlayerResources& res = resourceMgr->GetPlayerResources(playerId);
    
    // Train more peasants if needed
    int targetPeasants = std::min(res.maxPopulation / 2, 20);
    if (aiState->GetPeasantCount() < targetPeasants && 
        res.currentPopulation < res.maxPopulation) {
        TrainPeasants(map, resourceMgr);
    }
    
    // Assign idle workers to tasks
    if (aiState->GetIdleWorkerCount() > 0) {
        AssignPeasantsToResources(map, commandSystem);
    }
    
    // Build economic structures
    if (resourceMgr->CanAfford(playerId, ResourceCost(100, 0, 0, 0))) {
        BuildEconomicStructures(map, resourceMgr, techTree, commandSystem);
    }
}

void AIController::AssignPeasantsToResources(Map* map, CommandSystem* commandSystem) {
    std::vector<Unit*> myUnits = GetMyUnits(map);
    
    for (Unit* unit : myUnits) {
        if (unit->GetUnitType() != UnitType::PEASANT) continue;
        if (unit->GetState() != UnitState::IDLE) continue;
        
        // Find nearest resource
        Point2D unitPos = unit->GetGridPosition();
        Point2D resourcePos = FindResourceLocation(map, ResourceType::WOOD);
        
        if (resourcePos.x >= 0) {
            std::vector<Unit*> units = {unit};
            commandSystem->IssueGather(units, resourcePos, false);
        }
    }
}

void AIController::BuildEconomicStructures(Map* map, ResourceManager* resourceMgr,
                                          TechTree* techTree, CommandSystem* commandSystem) {
    BuildItem* nextBuild = buildOrder->GetNextBuild();
    if (!nextBuild) return;
    
    // Check if we can afford it
    ResourceCost cost = ResourceManager::GetBuildingCost(nextBuild->buildingType);
    if (!resourceMgr->CanAfford(playerId, cost)) return;
    
    // Check if tech allows it
    std::vector<BuildingType> available = techTree->GetAvailableBuildings(playerId);
    auto it = std::find(available.begin(), available.end(), nextBuild->buildingType);
    if (it == available.end()) return;
    
    // Find a builder
    std::vector<Unit*> myUnits = GetMyUnits(map);
    Unit* builder = nullptr;
    
    for (Unit* unit : myUnits) {
        if (unit->GetUnitType() == UnitType::PEASANT && 
            unit->GetState() == UnitState::IDLE) {
            builder = unit;
            break;
        }
    }
    
    if (!builder) return;
    
    // Find build location
    Point2D buildPos = FindBuildLocation(map, nextBuild->buildingType);
    if (buildPos.x < 0) return;
    
    // Issue build command
    commandSystem->IssueBuild(builder, nextBuild->buildingType, buildPos);
    
    // Spend resources
    resourceMgr->SpendResources(playerId, cost);
    
    // Mark as completed in build order (will be removed when actually built)
    buildOrder->MarkCompleted(nextBuild->buildingType);
}

void AIController::ManageMilitary(Map* map, ResourceManager* resourceMgr,
                                 TechTree* techTree, CommandSystem* commandSystem) {
    const PlayerResources& res = resourceMgr->GetPlayerResources(playerId);
    
    // Determine target military strength based on game phase
    int targetMilitary = 5;
    if (currentPhase == AIGamePhase::MID_GAME) {
        targetMilitary = 10;
    } else if (currentPhase == AIGamePhase::LATE_GAME) {
        targetMilitary = 20;
    }
    
    // Train military units if below target
    if (aiState->GetMilitaryUnitCount() < targetMilitary &&
        res.currentPopulation < res.maxPopulation) {
        TrainArmy(map, resourceMgr, techTree);
    }
    
    // Upgrade units if possible
    UpgradeUnits(map, resourceMgr, techTree);
}

void AIController::TrainPeasants(Map* map, ResourceManager* resourceMgr) {
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    
    for (Building* building : myBuildings) {
        if (building->GetBuildingType() != BuildingType::HUT) continue;
        if (!building->IsCompleted()) continue;
        if (building->IsProducingUnit()) continue;
        
        ResourceCost cost = ResourceManager::GetUnitCost(UnitType::PEASANT);
        if (resourceMgr->CanAfford(playerId, cost)) {
            building->ProduceUnit(UnitType::PEASANT);
            resourceMgr->SpendResources(playerId, cost);
            return;
        }
    }
}

void AIController::TrainArmy(Map* map, ResourceManager* resourceMgr, 
                            TechTree* techTree) {
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    
    for (Building* building : myBuildings) {
        if (building->GetBuildingType() != BuildingType::BARRACKS) continue;
        if (!building->IsCompleted()) continue;
        if (building->IsProducingUnit()) continue;
        
        ResourceCost cost = ResourceManager::GetUnitCost(UnitType::WARRIOR);
        if (resourceMgr->CanAfford(playerId, cost)) {
            building->ProduceUnit(UnitType::WARRIOR);
            resourceMgr->SpendResources(playerId, cost);
            return;
        }
    }
}

void AIController::UpgradeUnits(Map* map, ResourceManager* resourceMgr, 
                               TechTree* techTree) {
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    std::vector<Unit*> myUnits = GetMyUnits(map);
    
    // Find training ground
    Building* trainingGround = nullptr;
    for (Building* building : myBuildings) {
        if (building->GetBuildingType() == BuildingType::TRAINING_GROUND &&
            building->IsCompleted()) {
            trainingGround = building;
            break;
        }
    }
    
    if (!trainingGround) return;
    
    // Upgrade peasants to warriors if we have enough military
    if (aiState->GetMilitaryUnitCount() < 5) {
        for (Unit* unit : myUnits) {
            if (unit->GetUnitType() == UnitType::PEASANT &&
                aiState->GetPeasantCount() > 10) {
                ResourceCost cost = ResourceManager::GetUpgradeCost(
                    UnitType::PEASANT, UnitType::WARRIOR);
                if (resourceMgr->CanAfford(playerId, cost)) {
                    trainingGround->UpgradeUnit(unit);
                    resourceMgr->SpendResources(playerId, cost);
                    return;
                }
            }
        }
    }
    
    // Upgrade warriors to veterans
    for (Unit* unit : myUnits) {
        if (unit->GetUnitType() == UnitType::WARRIOR) {
            ResourceCost cost = ResourceManager::GetUpgradeCost(
                UnitType::WARRIOR, UnitType::VETERAN);
            if (resourceMgr->CanAfford(playerId, cost)) {
                trainingGround->UpgradeUnit(unit);
                resourceMgr->SpendResources(playerId, cost);
                return;
            }
        }
    }
    
    // Upgrade veterans to paladins
    for (Unit* unit : myUnits) {
        if (unit->GetUnitType() == UnitType::VETERAN) {
            ResourceCost cost = ResourceManager::GetUpgradeCost(
                UnitType::VETERAN, UnitType::PALADIN);
            if (resourceMgr->CanAfford(playerId, cost)) {
                trainingGround->UpgradeUnit(unit);
                resourceMgr->SpendResources(playerId, cost);
                return;
            }
        }
    }
}

void AIController::ManageTechTree(Map* map, ResourceManager* resourceMgr, 
                                 TechTree* techTree) {
    std::vector<std::string> availableTechs = techTree->GetAvailableTechs(playerId);
    
    if (availableTechs.empty()) return;
    
    // Prioritize certain techs based on game phase
    std::vector<std::string> priorityTechs;
    
    if (currentPhase == AIGamePhase::EARLY_GAME) {
        priorityTechs = {"Axes", "Urban planning"};
    } else if (currentPhase == AIGamePhase::MID_GAME) {
        priorityTechs = {"Mining", "Military", "Metal Processing"};
    } else {
        priorityTechs = {"Shields1", "Swords1", "Health1", "Tools1"};
    }
    
    // Try to research priority techs first
    for (const std::string& tech : priorityTechs) {
        auto it = std::find(availableTechs.begin(), availableTechs.end(), tech);
        if (it != availableTechs.end()) {
            ResourceCost cost = ResourceManager::GetTechCost(tech);
            if (resourceMgr->CanAfford(playerId, cost)) {
                // Find appropriate building to research
                Building* researchBuilding = FindResearchBuilding(map, tech);
                if (researchBuilding && !researchBuilding->IsResearchingTech()) {
                    researchBuilding->ResearchTech(tech);
                    resourceMgr->SpendResources(playerId, cost);
                    return;
                }
            }
        }
    }
}

Building* AIController::FindResearchBuilding(Map* map, const std::string& tech) {
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    
    for (Building* building : myBuildings) {
        if (!building->IsCompleted()) continue;
        if (!building->CanResearchTech()) continue;
        
        auto availableTechs = building->GetAvailableTechs();
        auto it = std::find(availableTechs.begin(), availableTechs.end(), tech);
        if (it != availableTechs.end()) {
            return building;
        }
    }
    
    return nullptr;
}

void AIController::AttackEnemy(Map* map, CommandSystem* commandSystem) {
    std::vector<Unit*> myMilitary;
    std::vector<Unit*> myUnits = GetMyUnits(map);
    
    for (Unit* unit : myUnits) {
        if (unit->GetUnitType() != UnitType::PEASANT) {
            myMilitary.push_back(unit);
        }
    }
    
    if (myMilitary.size() < 5) return; // Wait for more units
    
    // Find enemy target
    std::vector<Building*> enemyBuildings = GetEnemyBuildings(map);
    std::vector<Unit*> enemyUnits = GetEnemyUnits(map);
    
    Entity* target = nullptr;
    
    // Prioritize units over buildings
    if (!enemyUnits.empty()) {
        target = enemyUnits[0];
    } else if (!enemyBuildings.empty()) {
        target = enemyBuildings[0];
    }
    
    if (target) {
        commandSystem->IssueAttack(myMilitary, target, false);
    }
}

void AIController::DefendBase(Map* map, CommandSystem* commandSystem) {
    std::vector<Unit*> myMilitary;
    std::vector<Unit*> myUnits = GetMyUnits(map);
    
    for (Unit* unit : myUnits) {
        if (unit->GetUnitType() != UnitType::PEASANT) {
            myMilitary.push_back(unit);
        }
    }
    
    if (myMilitary.empty()) return;
    
    // Find enemies near our base
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    if (myBuildings.empty()) return;
    
    Point2D baseCenter = myBuildings[0]->GetGridPosition();
    
    std::vector<Unit*> enemyUnits = GetEnemyUnits(map);
    Unit* closestEnemy = nullptr;
    float closestDist = 99999.0f;
    
    for (Unit* enemy : enemyUnits) {
        Point2D enemyPos = enemy->GetGridPosition();
        int dx = enemyPos.x - baseCenter.x;
        int dy = enemyPos.y - baseCenter.y;
        float dist = sqrt(dx * dx + dy * dy);
        
        if (dist < closestDist) {
            closestDist = dist;
            closestEnemy = enemy;
        }
    }
    
    if (closestEnemy) {
        commandSystem->IssueAttack(myMilitary, closestEnemy, false);
    }
}

void AIController::ScoutMap(Map* map, CommandSystem* commandSystem) {
    std::vector<Unit*> myUnits = GetMyUnits(map);
    
    // Use one peasant as scout
    for (Unit* unit : myUnits) {
        if (unit->GetUnitType() == UnitType::PEASANT &&
            unit->GetState() == UnitState::IDLE) {
            
            Point2D scoutTarget = aiState->GetNextScoutTarget();
            std::vector<Unit*> scout = {unit};
            commandSystem->IssueMove(scout, scoutTarget, false);
            
            aiState->MarkAreaScouted(scoutTarget);
            return;
        }
    }
}

void AIController::BuildDefenses(Map* map, ResourceManager* resourceMgr,
                                TechTree* techTree, CommandSystem* commandSystem) {
    // Only build defenses in mid/late game
    if (currentPhase == AIGamePhase::EARLY_GAME) return;
    
    std::vector<BuildingType> available = techTree->GetAvailableBuildings(playerId);
    
    // Build towers
    auto it = std::find(available.begin(), available.end(), BuildingType::TOWER);
    if (it != available.end()) {
        ResourceCost cost = ResourceManager::GetBuildingCost(BuildingType::TOWER);
        if (resourceMgr->CanAfford(playerId, cost)) {
            // Find builder
            std::vector<Unit*> myUnits = GetMyUnits(map);
            for (Unit* unit : myUnits) {
                if (unit->GetUnitType() == UnitType::PEASANT &&
                    unit->GetState() == UnitState::IDLE) {
                    
                    Point2D buildPos = FindDefensivePosition(map);
                    if (buildPos.x >= 0) {
                        commandSystem->IssueBuild(unit, BuildingType::TOWER, buildPos);
                        resourceMgr->SpendResources(playerId, cost);
                    }
                    return;
                }
            }
        }
    }
}

Point2D AIController::FindDefensivePosition(Map* map) {
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    if (myBuildings.empty()) return Point2D(-1, -1);
    
    // Find perimeter of base
    Point2D baseCenter = myBuildings[0]->GetGridPosition();
    
    // Try positions around base
    for (int angle = 0; angle < 360; angle += 45) {
        float rad = angle * 3.14159f / 180.0f;
        int x = baseCenter.x + cos(rad) * 15;
        int y = baseCenter.y + sin(rad) * 15;
        
        if (map->CanPlaceBuilding(x, y, 2, 2)) {
            return Point2D(x, y);
        }
    }
    
    return Point2D(-1, -1);
}

bool AIController::ShouldAttack(Map* map) {
    // Attack if we have enough military and not under attack
    if (IsUnderAttack(map)) return false;
    
    int militaryCount = aiState->GetMilitaryUnitCount();
    
    if (difficulty == AIDifficulty::EASY && militaryCount >= 8) return true;
    if (difficulty == AIDifficulty::MEDIUM && militaryCount >= 6) return true;
    if (difficulty == AIDifficulty::HARD && militaryCount >= 5) return true;
    
    return false;
}

bool AIController::IsUnderAttack(Map* map) {
    return aiState->IsUnderAttack();
}

std::vector<Unit*> AIController::GetMyUnits(Map* map) {
    std::vector<Unit*> result;
    
    for (auto& entity : map->GetAllEntities()) {
        if (entity->GetType() == EntityType::UNIT &&
            entity->GetOwnerId() == playerId &&
            entity->IsAlive()) {
            result.push_back(static_cast<Unit*>(entity));
        }
    }
    
    return result;
}

std::vector<Building*> AIController::GetMyBuildings(Map* map) {
    std::vector<Building*> result;
    
    for (auto& entity : map->GetAllEntities()) {
        if (entity->GetType() == EntityType::BUILDING &&
            entity->GetOwnerId() == playerId &&
            entity->IsAlive()) {
            result.push_back(static_cast<Building*>(entity));
        }
    }
    
    return result;
}

std::vector<Unit*> AIController::GetEnemyUnits(Map* map) {
    std::vector<Unit*> result;
    
    for (auto& entity : map->GetAllEntities()) {
        if (entity->GetType() == EntityType::UNIT &&
            entity->GetOwnerId() != playerId &&
            entity->GetOwnerId() != -1 &&
            entity->IsAlive()) {
            result.push_back(static_cast<Unit*>(entity));
        }
    }
    
    return result;
}

std::vector<Building*> AIController::GetEnemyBuildings(Map* map) {
    std::vector<Building*> result;
    
    for (auto& entity : map->GetAllEntities()) {
        if (entity->GetType() == EntityType::BUILDING &&
            entity->GetOwnerId() != playerId &&
            entity->GetOwnerId() != -1 &&
            entity->IsAlive()) {
            result.push_back(static_cast<Building*>(entity));
        }
    }
    
    return result;
}

Point2D AIController::FindBuildLocation(Map* map, BuildingType type) {
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    
    if (myBuildings.empty()) {
        // First building - random location
        return Point2D(50, 50);
    }
    
    // Build near existing buildings
    Point2D baseCenter = myBuildings[0]->GetGridPosition();
    
    int buildingSize = 3; // Default size
    if (type == BuildingType::BARRACKS || type == BuildingType::TRAINING_GROUND) {
        buildingSize = 4;
    } else if (type == BuildingType::HUT || type == BuildingType::TOWER) {
        buildingSize = 2;
    } else if (type == BuildingType::ROAD || type == BuildingType::WALL) {
        buildingSize = 1;
    }
    
    // Try spiral pattern around base
    for (int radius = 5; radius < 30; radius += 5) {
        for (int angle = 0; angle < 360; angle += 30) {
            float rad = angle * 3.14159f / 180.0f;
            int x = baseCenter.x + cos(rad) * radius;
            int y = baseCenter.y + sin(rad) * radius;
            
            if (map->CanPlaceBuilding(x, y, buildingSize, buildingSize)) {
                return Point2D(x, y);
            }
        }
    }
    
    return Point2D(-1, -1);
}

Point2D AIController::FindResourceLocation(Map* map, ResourceType type) {
    std::vector<Building*> myBuildings = GetMyBuildings(map);
    
    if (myBuildings.empty()) {
        return Point2D(-1, -1);
    }
    
    Point2D baseCenter = myBuildings[0]->GetGridPosition();
    
    // Search for resources in expanding radius
    for (int radius = 5; radius < 50; radius += 5) {
        for (int angle = 0; angle < 360; angle += 15) {
            float rad = angle * 3.14159f / 180.0f;
            int x = baseCenter.x + cos(rad) * radius;
            int y = baseCenter.y + sin(rad) * radius;
            
            if (!map->IsInBounds(x, y)) continue;
            
            if (type == ResourceType::WOOD && map->HasTreeAt(x, y)) {
                return Point2D(x, y);
            }
            // Add metal and food checks here
        }
    }
    
    return Point2D(-1, -1);
}