#pragma once

#include "simulation/Agent.h" // for Vec2
#include <cstdint>

// Axis-aligned rectangular barrier
struct Barrier {
    uint32_t id = 0;
    Vec2 center;
    Vec2 halfExtents; // half width/height

    Barrier() = default;
    Barrier(uint32_t id, Vec2 center, Vec2 halfExtents);

    bool contains(Vec2 point) const;
    // Returns the closest point on the barrier surface to p
    Vec2 closestPoint(Vec2 p) const;
};