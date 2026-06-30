#include "Fighter.h"

Fighter::Fighter(int id, int ownerId, UnitType type)
    : Unit(id, ownerId, type)
    , combatExperience(0)
{
}

void Fighter::Update(float deltaTime) {
    Unit::Update(deltaTime);
    // Additional fighter-specific logic
}