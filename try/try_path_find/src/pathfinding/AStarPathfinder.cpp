#include "pathfinding/AStarPathfinder.h"
#include "simulation/World.h"
#include <algorithm>
#include <cmath>

thread_local AStarPathfinder::SearchContext AStarPathfinder::searchCtx_;

AStarPathfinder::SearchContext::SearchContext() {
    int total = Config::GRID_COLS * Config::GRID_ROWS;
    nodeData.resize(total);
    for (int i = 0; i < total; ++i) {
        nodeData[i].g = 1e18f;
        nodeData[i].parent = -1;
        nodeData[i].closed = false;
    }
    touchedNodes.reserve(4096);
    openHeap.reserve(4096);
}

void AStarPathfinder::SearchContext::reset() {
    for (int k : touchedNodes) {
        nodeData[k].g = 1e18f;
        nodeData[k].parent = -1;
        nodeData[k].closed = false;
    }
    touchedNodes.clear();
    openHeap.clear();
}

AStarPathfinder::AStarPathfinder() {}

void AStarPathfinder::onWorldChanged(const World& world) {
    std::lock_guard<std::mutex> lock(gridMutex_);
    gridDirty_ = true;
}

std::vector<Vec2> AStarPathfinder::findPath(Vec2 start, Vec2 end, const World& world) {
    // Rebuild grid if needed (only one thread does this)
    {
        std::lock_guard<std::mutex> lock(gridMutex_);
        if (gridDirty_) {
            grid_.rebuild(world);
            gridDirty_ = false;
        }
    }

    // From here on, grid_ is read-only — safe for concurrent access

    int sc, sr, ec, er;
    Grid::worldToGrid(start.x, start.y, sc, sr);
    Grid::worldToGrid(end.x, end.y, ec, er);

    if (grid_.isBlocked(ec, er)) return {end};

    int startKey = sr * Config::GRID_COLS + sc;
    int endKey = er * Config::GRID_COLS + ec;

    if (startKey == endKey) return {end};

    // Use thread-local search context
    SearchContext& ctx = searchCtx_;
    if (ctx.nodeData.empty()) {
        ctx = SearchContext(); // first use on this thread
    }
    ctx.reset();

    auto heuristic = [](int c1, int r1, int c2, int r2) -> float {
        int dx = std::abs(c1 - c2);
        int dy = std::abs(r1 - r2);
        return (float)(std::max(dx, dy)) + 0.414f * (float)(std::min(dx, dy));
    };

    ctx.touchedNodes.push_back(startKey);
    ctx.nodeData[startKey].g = 0.0f;
    ctx.nodeData[startKey].parent = -1;
    ctx.nodeData[startKey].closed = false;
    ctx.openHeap.push_back({startKey, heuristic(sc, sr, ec, er)});

    static const int dx[] = {1, -1, 0, 0, 1, -1, 1, -1};
    static const int dy[] = {0, 0, 1, -1, 1, 1, -1, -1};
    static const float dcost[] = {1.0f, 1.0f, 1.0f, 1.0f,
                                   1.414f, 1.414f, 1.414f, 1.414f};

    bool found = false;

    while (!ctx.openHeap.empty()) {
        std::pop_heap(ctx.openHeap.begin(), ctx.openHeap.end(),
                      std::greater<SearchContext::HeapEntry>());
        auto cur = ctx.openHeap.back();
        ctx.openHeap.pop_back();

        int curKey = cur.node;
        if (ctx.nodeData[curKey].closed) continue;
        ctx.nodeData[curKey].closed = true;

        if (curKey == endKey) { found = true; break; }

        int cc = curKey % Config::GRID_COLS;
        int cr = curKey / Config::GRID_COLS;
        float curG = ctx.nodeData[curKey].g;

        for (int i = 0; i < 8; ++i) {
            int nc = cc + dx[i];
            int nr = cr + dy[i];

            if (nc < 0 || nc >= Config::GRID_COLS || nr < 0 || nr >= Config::GRID_ROWS)
                continue;
            if (grid_.isBlocked(nc, nr)) continue;

            if (i >= 4) {
                if (grid_.isBlocked(cc + dx[i], cr) || grid_.isBlocked(cc, cr + dy[i]))
                    continue;
            }

            int nk = nr * Config::GRID_COLS + nc;
            if (ctx.nodeData[nk].closed) continue;

            float ng = curG + dcost[i];

            if (ctx.nodeData[nk].g >= 1e17f) {
                ctx.touchedNodes.push_back(nk);
            }

            if (ng < ctx.nodeData[nk].g) {
                ctx.nodeData[nk].g = ng;
                ctx.nodeData[nk].parent = curKey;
                float f = ng + heuristic(nc, nr, ec, er);
                ctx.openHeap.push_back({nk, f});
                std::push_heap(ctx.openHeap.begin(), ctx.openHeap.end(),
                               std::greater<SearchContext::HeapEntry>());
            }
        }
    }

    if (!found) return {end};

    std::vector<Vec2> path;
    int ck = endKey;
    while (ck != startKey && ck != -1) {
        int r = ck / Config::GRID_COLS;
        int c = ck % Config::GRID_COLS;
        float wx, wy;
        Grid::gridToWorld(c, r, wx, wy);
        path.push_back({wx, wy});
        ck = ctx.nodeData[ck].parent;
    }
    std::reverse(path.begin(), path.end());

    if (!path.empty()) path.back() = end;
    else path.push_back(end);

    // Path smoothing
    if (path.size() > 2) {
        std::vector<Vec2> smoothed;
        smoothed.push_back(path[0]);
        size_t current = 0;
        while (current < path.size() - 1) {
            size_t farthest = current + 1;
            for (size_t test = current + 2; test < path.size(); ++test) {
                if (!world.lineIntersectsBarrier(path[current], path[test])) {
                    farthest = test;
                }
            }
            smoothed.push_back(path[farthest]);
            current = farthest;
        }
        path = smoothed;
    }

    return path;
}