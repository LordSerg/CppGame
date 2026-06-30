#include "CombatSystem.h"
#include "../Map/Map.h"
#include "../Entities/Unit.h"
#include "../Entities/Building.h"

CombatSystem::CombatSystem(Map* map)
    : map(map)
{
}

void CombatSystem::Update(float deltaTime) {
    // Combat is mostly handled by units themselves
    // This could handle special combat effects, damage over time, etc.
}

int CombatSystem::CalculateDamage(Entity* attacker, Entity* defender) {
    if (!attacker || !defender) return 0;
    
    int baseDamage = 0;
    int armor = 0;
    
    if (attacker->GetType() == EntityType::UNIT) {
        Unit* unit = static_cast<Unit*>(attacker);
        baseDamage = unit->GetAttackDamage();
    } else if (attacker->GetType() == EntityType::BUILDING) {
        Building* building = static_cast<Building*>(attacker);
        baseDamage = building->GetAttackDamage();
    }
    
    if (defender->GetType() == EntityType::UNIT) {
        Unit* unit = static_cast<Unit*>(defender);
        armor = unit->GetArmor();
    }
    
    int finalDamage = baseDamage - armor;
    return std::max(1, finalDamage); // Minimum 1 damage
}

bool CombatSystem::IsInRange(Entity* attacker, Entity* target) {
    if (!attacker || !target) return false;
    
    int range = 1;
    
    if (attacker->GetType() == EntityType::UNIT) {
        Unit* unit = static_cast<Unit*>(attacker);
        range = unit->GetAttackRange();
    } else if (attacker->GetType() == EntityType::BUILDING) {
        Building* building = static_cast<Building*>(attacker);
        range = building->GetAttackRange();
    }
    
    float distance = attacker->GetPosition().Distance(target->GetPosition());
    return distance <= range * 32.0f;
}

void CombatSystem::DealAreaDamage(const Vector2& center, float radius, 
                                 int damage, int attackerPlayerId) {
    if (!map) return;
    
    // Find all entities in radius
    Rect searchArea(
        (int)(center.x / 32.0f - radius),
        (int)(center.y / 32.0f - radius),
        (int)(radius * 2),
        (int)(radius * 2)
    );
    
    auto entities = map->GetEntitiesInRect(searchArea);
    
    for (Entity* entity : entities) {
        if (entity->GetOwnerId() == attackerPlayerId) continue;
        
        float distance = center.Distance(entity->GetPosition());
        if (distance <= radius * 32.0f) {
            entity->TakeDamage(damage);
        }
    }
}

void CombatSystem::ProcessCombat(Entity* attacker, Entity* defender, float deltaTime) {
    if (!IsInRange(attacker, defender)) return;
    
    int damage = CalculateDamage(attacker, defender);
    defender->TakeDamage(damage);
}