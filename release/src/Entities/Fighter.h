#ifndef FIGHTER_H
#define FIGHTER_H

#include "Unit.h"

class Fighter : public Unit {
public:
    Fighter(int id, int ownerId, UnitType type = UnitType::WARRIOR);
    
    void Update(float deltaTime) override;
    
private:
    float combatExperience;
};

#endif // FIGHTER_H