#pragma once

#include "steering/ISteeringBehavior.h"

// Boids-like flocking: separation + alignment + cohesion
class FlockingBehavior : public ISteeringBehavior {
public:
    void apply(std::vector<Agent>& agents, const World& world, float dt) override;
    std::string name() const override { return "Flocking"; }

    float separationWeight = 2.0f;
    float alignmentWeight = 1.0f;
    float cohesionWeight = 0.5f;
    float neighborRadius = 40.0f;
};