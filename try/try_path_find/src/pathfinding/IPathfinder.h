#pragma once

#include "simulation/Agent.h"
#include <vector>
#include <string>

class World;

class IPathfinder {
public:
    virtual ~IPathfinder() = default;
    virtual std::vector<Vec2> findPath(Vec2 start, Vec2 end, const World& world) = 0;
    virtual std::string name() const = 0;

    // Called when barriers change so pathfinder can rebuild internal structures
    virtual void onWorldChanged(const World& world) {}
};