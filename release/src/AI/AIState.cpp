#include "AIState.h"

AIState::AIState()
    : underAttack(false)
    , attackLocation(0, 0)
    , peasantCount(0)
    , idleWorkerCount(0)
    , militaryUnitCount(0)
    , idleMilitaryCount(0)
    , timeSinceLastAttack(0)
{
}

void AIState::Update(float deltaTime) {
    // Update enemy information
    for (auto& enemy : enemies) {
        enemy.timeSinceLastSeen += deltaTime;
        
        // Decay threat over time if not seen
        if (enemy.timeSinceLastSeen > 10.0f) {
            enemy.threatLevel *= 0.99f;
        }
    }
    
    // Update attack status
    if (underAttack) {
        timeSinceLastAttack += deltaTime;
        if (timeSinceLastAttack > 30.0f) {
            underAttack = false;
        }
    }
}

void AIState::UpdateEnemyInfo(int playerId, const Point2D& position, 
                             int unitCount, int buildingCount) {
    EnemyInfo* info = GetEnemyInfo(playerId);
    
    if (!info) {
        enemies.push_back(EnemyInfo());
        info = &enemies.back();
        info->playerId = playerId;
    }
    
    info->lastKnownPosition = position;
    info->unitCount = unitCount;
    info->buildingCount = buildingCount;
    info->timeSinceLastSeen = 0;
    
    // Calculate threat level
    info->threatLevel = unitCount * 1.0f + buildingCount * 0.5f;
}

EnemyInfo* AIState::GetEnemyInfo(int playerId) {
    for (auto& enemy : enemies) {
        if (enemy.playerId == playerId) {
            return &enemy;
        }
    }
    return nullptr;
}

void AIState::MarkAreaScouted(const Point2D& position) {
    // Check if area already scouted
    for (auto& area : scoutedAreas) {
        int dx = area.position.x - position.x;
        int dy = area.position.y - position.y;
        if (dx * dx + dy * dy < 100) { // Within 10 tiles
            area.timeExplored = 0;
            return;
        }
    }
    
    // Add new scouted area
    ScoutInfo info;
    info.position = position;
    info.timeExplored = 0;
    scoutedAreas.push_back(info);
}

bool AIState::IsAreaScouted(const Point2D& position) const {
    for (const auto& area : scoutedAreas) {
        int dx = area.position.x - position.x;
        int dy = area.position.y - position.y;
        if (dx * dx + dy * dy < 100 && area.timeExplored < 300.0f) {
            return true;
        }
    }
    return false;
}

Point2D AIState::GetNextScoutTarget() const {
    // Simple spiral pattern scouting
    // In a real implementation, this would be more sophisticated
    return Point2D(rand() % 1000, rand() % 1000);
}

float AIState::GetTotalThreat() const {
    float total = 0;
    for (const auto& enemy : enemies) {
        total += enemy.threatLevel;
    }
    return total;
}