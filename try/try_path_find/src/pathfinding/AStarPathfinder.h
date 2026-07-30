#pragma once

#include "pathfinding/IPathfinder.h"
#include "pathfinding/Grid.h"
#include <vector>
#include <mutex>

class AStarPathfinder : public IPathfinder {
public:
    AStarPathfinder();
    std::vector<Vec2> findPath(Vec2 start, Vec2 end, const World& world) override;
    std::string name() const override { return "A*"; }
    void onWorldChanged(const World& world) override;

    // Thread-safe: grid is read-only during searches
    const Grid& getGrid() const { return grid_; }

private:
    Grid grid_;
    bool gridDirty_ = true;
    std::mutex gridMutex_; // protects grid rebuild only

    // Per-search working memory (allocated per call, not shared)
    struct SearchContext {
        struct NodeData {
            float g = 1e18f;
            int parent = -1;
            bool closed = false;
        };
        std::vector<NodeData> nodeData;
        std::vector<int> touchedNodes;

        struct HeapEntry {
            int node;
            float f;
            bool operator>(const HeapEntry& o) const { return f > o.f; }
        };
        std::vector<HeapEntry> openHeap;

        SearchContext();
        void reset();
    };

    // Thread-local search contexts
    static thread_local SearchContext searchCtx_;
};