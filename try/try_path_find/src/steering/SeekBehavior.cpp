#include "steering/SeekBehavior.h"
#include "simulation/Agent.h"

void SeekBehavior::apply(std::vector<Agent>& agents, const World& world, float dt) {
    // No modification - pure seek is already handled in World::update
    (void)agents; (void)world; (void)dt;
}