#pragma once

#include <vector>
#include <string>

struct Agent;
class World;

class ISteeringBehavior {
public:
    virtual ~ISteeringBehavior() = default;
    // Modify agent velocities in-place
    virtual void apply(std::vector<Agent>& agents, const World& world, float dt) = 0;
    virtual std::string name() const = 0;
};