#ifndef COMBATSYSTEM_H
#define COMBATSYSTEM_H

#include <vector>

class Map;
class Entity;

class CombatSystem {
public:
    CombatSystem(Map* map);
    
    void Update(float deltaTime);
    
    // Combat calculations
    int CalculateDamage(Entity* attacker, Entity* defender);
    bool IsInRange(Entity* attacker, Entity* target);
    
    // Area effects
    void DealAreaDamage(const class Vector2& center, float radius, 
                       int damage, int attackerPlayerId);
    
private:
    Map* map;
    
    void ProcessCombat(Entity* attacker, Entity* defender, float deltaTime);
};

#endif // COMBATSYSTEM_H