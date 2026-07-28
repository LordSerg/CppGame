#pragma once

#include "pathfinding/IPathfinder.h"
#include "pathfinding/Grid.h"

// Theta* - any-angle pathfinding (A* variant with line-of-sight checks)
class ThetaStarPathfinder : public IPathfinder {
public:
    std::vector<Vec2> findPath(Vec2 start, Vec2 end, const World& world) override;
    std::string name() const override { return "Theta*"; }
    void onWorldChanged(const World& world) override { gridDirty_ = true; }

private:
    bool lineOfSight(int c1, int r1, int c2, int r2) const;
    Grid grid_;
    bool gridDirty_ = true;
};