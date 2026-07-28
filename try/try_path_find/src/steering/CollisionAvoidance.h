#pragma once

#include "steering/ISteeringBehavior.h"

// Predictive collision avoidance: looks ahead and steers to avoid future collisions
class CollisionAvoidance : public ISteeringBehavior {
public:
    void apply(std::vector<Agent>& agents, const World& world, float dt) override;
    std::string name() const override { return "Collision Avoidance"; }

    float lookAheadTime = 1.5f;
    float avoidanceStrength = 100.0f;
};