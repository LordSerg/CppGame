#ifndef AISTATE_H
#define AISTATE_H

#include "../Utils/Math.h"
#include <vector>

struct EnemyInfo {
    int playerId;
    Point2D lastKnownPosition;
    int unitCount;
    int buildingCount;
    float threatLevel;
    float timeSinceLastSeen;
    
    EnemyInfo() 
        : playerId(-1)
        , lastKnownPosition(0, 0)
        , unitCount(0)
        , buildingCount(0)
        , threatLevel(0)
        , timeSinceLastSeen(0)
    {}
};

struct ScoutInfo {
    Point2D position;
    float timeExplored;
    bool hasResources;
    bool hasEnemy;
    
    ScoutInfo()
        : position(0, 0)
        , timeExplored(0)
        , hasResources(false)
        , hasEnemy(false)
    {}
};

class AIState {
public:
    AIState();
    
    void Update(float deltaTime);
    
    // Enemy tracking
    void UpdateEnemyInfo(int playerId, const Point2D& position, 
                        int unitCount, int buildingCount);
    EnemyInfo* GetEnemyInfo(int playerId);
    
    // Scouting
    void MarkAreaScouted(const Point2D& position);
    bool IsAreaScouted(const Point2D& position) const;
    Point2D GetNextScoutTarget() const;
    
    // Threat assessment
    float GetTotalThreat() const;
    bool IsUnderAttack() const { return underAttack; }
    void SetUnderAttack(bool value) { underAttack = value; }
    
    // Economy tracking
    int GetPeasantCount() const { return peasantCount; }
    void SetPeasantCount(int count) { peasantCount = count; }
    
    int GetIdleWorkerCount() const { return idleWorkerCount; }
    void SetIdleWorkerCount(int count) { idleWorkerCount = count; }
    
    // Military tracking
    int GetMilitaryUnitCount() const { return militaryUnitCount; }
    void SetMilitaryUnitCount(int count) { militaryUnitCount = count; }
    
    int GetIdleMilitaryCount() const { return idleMilitaryCount; }
    void SetIdleMilitaryCount(int count) { idleMilitaryCount = count; }
    
private:
    std::vector<EnemyInfo> enemies;
    std::vector<ScoutInfo> scoutedAreas;
    
    bool underAttack;
    Point2D attackLocation;
    
    int peasantCount;
    int idleWorkerCount;
    int militaryUnitCount;
    int idleMilitaryCount;
    
    float timeSinceLastAttack;
};

#endif // AISTATE_H