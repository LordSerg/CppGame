#pragma once

#include "steering/ISteeringBehavior.h"

// No additional steering - agents just follow their path
class SeekBehavior : public ISteeringBehavior {
public:
    void apply(std::vector<Agent>& agents, const World& world, float dt) override;
    std::string name() const override { return "Seek Only"; }
};