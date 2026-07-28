#include "pathfinding/FlowFieldPathfinder.h"
#include "simulation/World.h"
#include <queue>
#include <cmath>
#include <limits>

void FlowFieldPathfinder::buildField(int tc, int tr) {
    int cols = Config::GRID_COLS, rows = Config::GRID_ROWS;
    int total = cols * rows;
    costField_.assign(total, std::numeric_limits<float>::max());
    flowField_.assign(total, Vec2(0, 0));

    auto key = [cols](int c, int r) { return r * cols + c; };

    std::queue<std::pair<int,int>> frontier;
    costField_[key(tc, tr)] = 0;
    frontier.push({tc, tr});

    static const int dx[] = {1,-1,0,0,1,-1,1,-1};
    static const int dy[] = {0,0,1,-1,1,1,-1,-1};
    static const float dcost[] = {1,1,1,1,1.414f,1.414f,1.414f,1.414f};

    while (!frontier.empty()) {
        auto [cc, cr] = frontier.front(); frontier.pop();
        float curCost = costField_[key(cc, cr)];
        for (int i = 0; i < 8; ++i) {
            int nc = cc + dx[i], nr = cr + dy[i];
            if (nc < 0 || nc >= cols || nr < 0 || nr >= rows) continue;
            if (grid_.isBlocked(nc, nr)) continue;
            float nc2 = curCost + dcost[i];
            int nk = key(nc, nr);
            if (nc2 < costField_[nk]) {
                costField_[nk] = nc2;
                frontier.push({nc, nr});
            }
        }
    }

    // Build flow directions
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            if (grid_.isBlocked(c, r)) continue;
            float bestCost = costField_[key(c, r)];
            Vec2 bestDir(0, 0);
            for (int i = 0; i < 8; ++i) {
                int nc = c + dx[i], nr = r + dy[i];
                if (nc < 0 || nc >= cols || nr < 0 || nr >= rows) continue;
                if (grid_.isBlocked(nc, nr)) continue;
                float cost = costField_[key(nc, nr)];
                if (cost < bestCost) {
                    bestCost = cost;
                    bestDir = Vec2((float)dx[i], (float)dy[i]).normalized();
                }
            }
            flowField_[key(c, r)] = bestDir;
        }
    }

    lastTargetCol_ = tc;
    lastTargetRow_ = tr;
}

std::vector<Vec2> FlowFieldPathfinder::findPath(Vec2 start, Vec2 end, const World& world) {
    if (gridDirty_) { grid_.rebuild(world); gridDirty_ = false; }

    int tc, tr;
    Grid::worldToGrid(end.x, end.y, tc, tr);

    if (tc != lastTargetCol_ || tr != lastTargetRow_) {
        buildField(tc, tr);
    }

    // Trace path from start following flow field
    std::vector<Vec2> path;
    int cc, cr;
    Grid::worldToGrid(start.x, start.y, cc, cr);

    int maxSteps = Config::GRID_COLS * Config::GRID_ROWS;
    for (int step = 0; step < maxSteps; ++step) {
        if (cc == tc && cr == tr) break;
        int k = cr * Config::GRID_COLS + cc;
        Vec2 dir = flowField_[k];
        if (dir.lengthSq() < 1e-6f) break; // stuck

        int nc = cc + (int)std::round(dir.x);
        int nr = cr + (int)std::round(dir.y);
        if (nc == cc && nr == cr) break;

        float wx, wy;
        Grid::gridToWorld(nc, nr, wx, wy);
        path.push_back({wx, wy});
        cc = nc;
        cr = nr;
    }

    if (!path.empty()) path.back() = end;
    else path.push_back(end);
    return path;
}