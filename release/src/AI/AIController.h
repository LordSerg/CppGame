#ifndef AICONTROLLER_H
#define AICONTROLLER_H

#include "../Systems/ResourceManager.h"
#include "../Systems/TechTree.h"
#include "BuildOrder.h"
#include <memory>
#include <vector>

enum class AIGamePhase {
    EARLY_GAME,
    MID_GAME,
    LATE_GAME,
    ATTACKING,
    DEFENDING
};

enum class AIDifficulty {
    EASY,
    MEDIUM,
    HARD
};

class AIController {
public:
    AIController(int playerId, AIDifficulty difficulty);
    ~AIController();
    
    void Update(float deltaTime, class Map* map, 
                class ResourceManager* resourceMgr,
                class TechTree* techTree,
                class CommandSystem* commandSystem);
    
    void SetDifficulty(AIDifficulty diff) { difficulty = diff; }
    
private:
    int playerId;
    AIDifficulty difficulty;
    AIGamePhase currentPhase;  // Changed from currentState
    
    float timeSinceLastDecision;
    float decisionInterval;
    
    std::unique_ptr<BuildOrder> buildOrder;
    std::unique_ptr<class AIState> aiState;  // This is the class
    
    // Decision making
    void MakeDecisions(class Map* map, 
                      class ResourceManager* resourceMgr,
                      class TechTree* techTree,
                      class CommandSystem* commandSystem);
    
    void UpdateAIState(class Map* map);
    void DetermineGamePhase(class Map* map, class ResourceManager* resourceMgr);
    
    // Economy
    void ManageEconomy(class Map* map, 
                      class ResourceManager* resourceMgr,
                      class TechTree* techTree,
                      class CommandSystem* commandSystem);
    
    void AssignPeasantsToResources(class Map* map, class CommandSystem* commandSystem);
    void BuildEconomicStructures(class Map* map, 
                                 class ResourceManager* resourceMgr,
                                 class TechTree* techTree,
                                 class CommandSystem* commandSystem);
    
    void TrainPeasants(class Map* map, class ResourceManager* resourceMgr);
    
    // Military
    void ManageMilitary(class Map* map,
                       class ResourceManager* resourceMgr,
                       class TechTree* techTree,
                       class CommandSystem* commandSystem);
    
    void TrainArmy(class Map* map,
                  class ResourceManager* resourceMgr,
                  class TechTree* techTree);
    
    void AttackEnemy(class Map* map, class CommandSystem* commandSystem);
    void DefendBase(class Map* map, class CommandSystem* commandSystem);
    
    void UpgradeUnits(class Map* map, class ResourceManager* resourceMgr, 
                     class TechTree* techTree);
    
    // Tech
    void ManageTechTree(class Map* map,
                       class ResourceManager* resourceMgr,
                       class TechTree* techTree);
    
    class Building* FindResearchBuilding(class Map* map, const std::string& tech);
    
    // Scouting
    void ScoutMap(class Map* map, class CommandSystem* commandSystem);
    
    // Defense
    void BuildDefenses(class Map* map,
                      class ResourceManager* resourceMgr,
                      class TechTree* techTree,
                      class CommandSystem* commandSystem);
    
    Point2D FindDefensivePosition(class Map* map);
    
    // Helpers
    std::vector<class Unit*> GetMyUnits(class Map* map);
    std::vector<class Building*> GetMyBuildings(class Map* map);
    std::vector<class Unit*> GetEnemyUnits(class Map* map);
    std::vector<class Building*> GetEnemyBuildings(class Map* map);
    
    Point2D FindBuildLocation(class Map* map, BuildingType type);
    Point2D FindResourceLocation(class Map* map, ResourceType type);
    
    bool ShouldAttack(class Map* map);
    bool IsUnderAttack(class Map* map);
};

#endif // AICONTROLLER_H