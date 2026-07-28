#pragma once

#include "simulation/Agent.h"
#include <vector>
#include <string>

class IFormation {
public:
    virtual ~IFormation() = default;
    // Returns offset positions relative to the formation center for N agents
    virtual std::vector<Vec2> computeOffsets(int count) const = 0;
    virtual std::string name() const = 0;

    float spacing = 10.0f; // spacing between agents in formation
};