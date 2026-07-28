#pragma once

#include "pathfinding/IPathfinder.h"
#include "pathfinding/Grid.h"

class AStarPathfinder : public IPathfinder {
public:
    AStarPathfinder();
    std::vector<Vec2> findPath(Vec2 start, Vec2 end, const World& world) override;
    std::string name() const override { return "A*"; }
    void onWorldChanged(const World& world) override;

private:
    Grid grid_;
    bool gridDirty_ = true;
};