#pragma once

#include "pathfinding/IPathfinder.h"
#include "pathfinding/Grid.h"
#include <vector>

class AStarPathfinder : public IPathfinder {
public:
    AStarPathfinder();
    std::vector<Vec2> findPath(Vec2 start, Vec2 end, const World& world) override;
    std::string name() const override { return "A*"; }
    void onWorldChanged(const World& world) override;

private:
    Grid grid_;
    bool gridDirty_ = true;

    // Pre-allocated work buffers (avoid per-search allocation)
    struct NodeData {
        float g;
        int parent;
        bool closed;
    };
    std::vector<NodeData> nodeData_; // size = GRID_COLS * GRID_ROWS

    // Binary heap open set using indices
    struct HeapEntry {
        int node;
        float f;
        bool operator>(const HeapEntry& o) const { return f > o.f; }
    };
    std::vector<HeapEntry> openHeap_;
    std::vector<bool> inOpen_;

    // Track which nodes were touched so we only reset those
    std::vector<int> touchedNodes_;
};