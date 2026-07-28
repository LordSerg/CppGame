#include "simulation/Barrier.h"
#include <algorithm>

Barrier::Barrier(uint32_t id, Vec2 center, Vec2 halfExtents)
    : id(id), center(center), halfExtents(halfExtents) {}

bool Barrier::contains(Vec2 point) const {
    return (point.x >= center.x - halfExtents.x &&
            point.x <= center.x + halfExtents.x &&
            point.y >= center.y - halfExtents.y &&
            point.y <= center.y + halfExtents.y);
}

Vec2 Barrier::closestPoint(Vec2 p) const {
    float cx = std::max(center.x - halfExtents.x, std::min(p.x, center.x + halfExtents.x));
    float cy = std::max(center.y - halfExtents.y, std::min(p.y, center.y + halfExtents.y));
    return {cx, cy};
}