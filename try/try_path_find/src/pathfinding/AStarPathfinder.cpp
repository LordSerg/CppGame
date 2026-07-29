#include "pathfinding/AStarPathfinder.h"
#include "simulation/World.h"
#include <algorithm>
#include <cmath>
#include <queue>

AStarPathfinder::AStarPathfinder() {
    int total = Config::GRID_COLS * Config::GRID_ROWS;
    nodeData_.resize(total);
    inOpen_.resize(total, false);
    touchedNodes_.reserve(4096);
}

void AStarPathfinder::onWorldChanged(const World& world) {
    gridDirty_ = true;
}

std::vector<Vec2> AStarPathfinder::findPath(Vec2 start, Vec2 end, const World& world) {
    if (gridDirty_) {
        grid_.rebuild(world);
        gridDirty_ = false;
    }

    int sc, sr, ec, er;
    Grid::worldToGrid(start.x, start.y, sc, sr);
    Grid::worldToGrid(end.x, end.y, ec, er);

    if (grid_.isBlocked(ec, er)) return {end};

    int startKey = sr * Config::GRID_COLS + sc;
    int endKey = er * Config::GRID_COLS + ec;

    if (startKey == endKey) return {end};

    // Reset only previously touched nodes
    for (int k : touchedNodes_) {
        nodeData_[k].g = 1e18f;
        nodeData_[k].parent = -1;
        nodeData_[k].closed = false;
        inOpen_[k] = false;
    }
    touchedNodes_.clear();
    openHeap_.clear();

    auto touch = [this](int k) {
        if (nodeData_[k].g >= 1e17f) { // untouched
            touchedNodes_.push_back(k);
        }
    };

    // Heuristic (Octile distance - tighter than Euclidean for 8-directional)
    auto heuristic = [](int c1, int r1, int c2, int r2) -> float {
        int dx = std::abs(c1 - c2);
        int dy = std::abs(r1 - r2);
        return (float)(std::max(dx, dy)) + 0.414f * (float)(std::min(dx, dy));
    };

    touch(startKey);
    nodeData_[startKey].g = 0.0f;
    nodeData_[startKey].parent = -1;
    inOpen_[startKey] = true;
    openHeap_.push_back({startKey, heuristic(sc, sr, ec, er)});

    static const int dx[] = {1, -1, 0, 0, 1, -1, 1, -1};
    static const int dy[] = {0, 0, 1, -1, 1, 1, -1, -1};
    static const float dcost[] = {1.0f, 1.0f, 1.0f, 1.0f,
                                   1.414f, 1.414f, 1.414f, 1.414f};

    bool found = false;

    while (!openHeap_.empty()) {
        std::pop_heap(openHeap_.begin(), openHeap_.end(), std::greater<HeapEntry>());
        HeapEntry cur = openHeap_.back();
        openHeap_.pop_back();

        int curKey = cur.node;
        if (curKey == endKey) { found = true; break; }
        if (nodeData_[curKey].closed) continue;
        nodeData_[curKey].closed = true;
        inOpen_[curKey] = false;

        int cc = curKey % Config::GRID_COLS;
        int cr = curKey / Config::GRID_COLS;
        float curG = nodeData_[curKey].g;

        for (int i = 0; i < 8; ++i) {
            int nc = cc + dx[i];
            int nr = cr + dy[i];

            if (nc < 0 || nc >= Config::GRID_COLS || nr < 0 || nr >= Config::GRID_ROWS)
                continue;
            if (grid_.isBlocked(nc, nr)) continue;

            // Corner cutting check for diagonals
            if (i >= 4) {
                if (grid_.isBlocked(cc + dx[i], cr) || grid_.isBlocked(cc, cr + dy[i]))
                    continue;
            }

            int nk = nr * Config::GRID_COLS + nc;
            touch(nk);

            if (nodeData_[nk].closed) continue;

            float ng = curG + dcost[i];
            if (ng < nodeData_[nk].g) {
                nodeData_[nk].g = ng;
                nodeData_[nk].parent = curKey;
                float f = ng + heuristic(nc, nr, ec, er);
                openHeap_.push_back({nk, f});
                std::push_heap(openHeap_.begin(), openHeap_.end(), std::greater<HeapEntry>());
                inOpen_[nk] = true;
            }
        }
    }

    if (!found) return {end};

    // Reconstruct
    std::vector<Vec2> path;
    int ck = endKey;
    while (ck != startKey && ck != -1) {
        int r = ck / Config::GRID_COLS;
        int c = ck % Config::GRID_COLS;
        float wx, wy;
        Grid::gridToWorld(c, r, wx, wy);
        path.push_back({wx, wy});
        ck = nodeData_[ck].parent;
    }
    std::reverse(path.begin(), path.end());

    if (!path.empty()) path.back() = end;
    else path.push_back(end);

    // Path smoothing: remove unnecessary waypoints with line-of-sight
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