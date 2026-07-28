#pragma once

#include "steering/ISteeringBehavior.h"

class SeparationBehavior : public ISteeringBehavior {
public:
    void apply(std::vector<Agent>& agents, const World& world, float dt) override;
    std::string name() const override { return "Separation"; }

    float separationRadius = 30.0f;
    float separationStrength = 80.0f;
};