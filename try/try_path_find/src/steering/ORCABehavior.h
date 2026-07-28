#pragma once

#include "steering/ISteeringBehavior.h"

// Simplified ORCA (Optimal Reciprocal Collision Avoidance)
// Full ORCA requires half-plane intersection; this is a simplified version
class ORCABehavior : public ISteeringBehavior {
public:
    void apply(std::vector<Agent>& agents, const World& world, float dt) override;
    std::string name() const override { return "ORCA (Simplified)"; }

    float timeHorizon = 2.0f;
};