#include "pathfinding/AStarPathfinder.h"
#include "simulation/World.h"
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cmath>

AStarPathfinder::AStarPathfinder() {}

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

    if (grid_.isBlocked(ec, er)) {
        // Target in a wall, just return direct
        return {end};
    }

    struct Node {
        int col, row;
        float g, f;
        bool operator>(const Node& o) const { return f > o.f; }
    };

    auto key = [](int c, int r) { return r * Config::GRID_COLS + c; };

    auto heuristic = [](int c1, int r1, int c2, int r2) -> float {
        float dx = (float)(c1 - c2);
        float dy = (float)(r1 - r2);
        return std::sqrt(dx * dx + dy * dy);
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    std::unordered_map<int, float> gScore;
    std::unordered_map<int, int> cameFrom;

    int startKey = key(sc, sr);
    int endKey = key(ec, er);

    open.push({sc, sr, 0.0f, heuristic(sc, sr, ec, er)});
    gScore[startKey] = 0.0f;

    static const int dx[] = {1, -1, 0, 0, 1, -1, 1, -1};
    static const int dy[] = {0, 0, 1, -1, 1, 1, -1, -1};
    static const float dcost[] = {1.0f, 1.0f, 1.0f, 1.0f,
                                   1.414f, 1.414f, 1.414f, 1.414f};

    bool found = false;

    while (!open.empty()) {
        Node cur = open.top();
        open.pop();

        int curKey = key(cur.col, cur.row);
        if (curKey == endKey) {
            found = true;
            break;
        }

        if (cur.g > gScore[curKey] + 1e-4f) continue;

        for (int i = 0; i < 8; ++i) {
            int nc = cur.col + dx[i];
            int nr = cur.row + dy[i];

            if (grid_.isBlocked(nc, nr)) continue;

            // For diagonal, both adjacent cardinal cells must be free
            if (i >= 4) {
                if (grid_.isBlocked(cur.col + dx[i], cur.row) ||
                    grid_.isBlocked(cur.col, cur.row + dy[i]))
                    continue;
            }

            float ng = cur.g + dcost[i];
            int nk = key(nc, nr);

            if (gScore.find(nk) == gScore.end() || ng < gScore[nk]) {
                gScore[nk] = ng;
                cameFrom[nk] = curKey;
                float f = ng + heuristic(nc, nr, ec, er);
                open.push({nc, nr, ng, f});
            }
        }
    }

    if (!found) return {end}; // No path, just aim directly

    // Reconstruct path
    std::vector<Vec2> path;
    int ck = endKey;
    while (ck != startKey) {
        int r = ck / Config::GRID_COLS;
        int c = ck % Config::GRID_COLS;
        float wx, wy;
        Grid::gridToWorld(c, r, wx, wy);
        path.push_back({wx, wy});
        ck = cameFrom[ck];
    }
    std::reverse(path.begin(), path.end());

    // Replace last waypoint with exact target
    if (!path.empty()) path.back() = end;
    else path.push_back(end);

    return path;
}