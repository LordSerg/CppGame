#pragma once

#include "pathfinding/IPathfinder.h"
#include "pathfinding/Grid.h"

class DijkstraPathfinder : public IPathfinder {
public:
    std::vector<Vec2> findPath(Vec2 start, Vec2 end, const World& world) override;
    std::string name() const override { return "Dijkstra"; }
    void onWorldChanged(const World& world) override { gridDirty_ = true; }

private:
    Grid grid_;
    bool gridDirty_ = true;
};