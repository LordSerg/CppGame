#ifndef PEASANT_H
#define PEASANT_H

#include "Unit.h"

class Peasant : public Unit {
public:
    Peasant(int id, int ownerId);
    
    void Update(float deltaTime) override;
    
private:
    float miningEfficiency;
    float buildingEfficiency;
};

#endif // PEASANT_H