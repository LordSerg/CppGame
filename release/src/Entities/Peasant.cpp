#include "Peasant.h"

Peasant::Peasant(int id, int ownerId)
    : Unit(id, ownerId, UnitType::PEASANT)
    , miningEfficiency(1.0f)
    , buildingEfficiency(1.0f)
{
}

void Unit::Update(float deltaTime) {
    Unit::Update(deltaTime);
    // Additional peasant-specific logic
}