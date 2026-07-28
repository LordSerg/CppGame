#include "pathfinding/ThetaStarPathfinder.h"
#include "simulation/World.h"
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cmath>

bool ThetaStarPathfinder::lineOfSight(int c1, int r1, int c2, int r2) const {
    // Bresenham line check
    int dc = std::abs(c2 - c1), dr = std::abs(r2 - r1);
    int sc = (c1 < c2) ? 1 : -1;
    int sr = (r1 < r2) ? 1 : -1;
    int err = dc - dr;
    int c = c1, r = r1;

    while (true) {
        if (grid_.isBlocked(c, r)) return false;
        if (c == c2 && r == r2) break;
        int e2 = 2 * err;
        if (e2 > -dr) { err -= dr; c += sc; }
        if (e2 < dc) { err += dc; r += sr; }
    }
    return true;
}

std::vector<Vec2> ThetaStarPathfinder::findPath(Vec2 start, Vec2 end, const World& world) {
    if (gridDirty_) { grid_.rebuild(world); gridDirty_ = false; }

    int sc, sr, ec, er;
    Grid::worldToGrid(start.x, start.y, sc, sr);
    Grid::worldToGrid(end.x, end.y, ec, er);
    if (grid_.isBlocked(ec, er)) return {end};

    struct Node {
        int col, row;
        float g, f;
        bool operator>(const Node& o) const { return f > o.f; }
    };

    auto key = [](int c, int r) { return r * Config::GRID_COLS + c; };
    auto heuristic = [](int c1, int r1, int c2, int r2) -> float {
        float dx = (float)(c1 - c2), dy = (float)(r1 - r2);
        return std::sqrt(dx * dx + dy * dy);
    };

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    std::unordered_map<int, float> gScore;
    std::unordered_map<int, int> cameFrom;
    // parent stores the actual parent (not grid neighbor) for Theta*
    std::unordered_map<int, int> parent;

    int startKey = key(sc, sr);
    int endKey = key(ec, er);
    open.push({sc, sr, 0.0f, heuristic(sc, sr, ec, er)});
    gScore[startKey] = 0.0f;
    parent[startKey] = startKey;

    static const int dx[] = {1,-1,0,0,1,-1,1,-1};
    static const int dy[] = {0,0,1,-1,1,1,-1,-1};

    bool found = false;
    while (!open.empty()) {
        Node cur = open.top(); open.pop();
        int curKey = key(cur.col, cur.row);
        if (curKey == endKey) { found = true; break; }
        if (cur.g > gScore[curKey] + 1e-4f) continue;

        for (int i = 0; i < 8; ++i) {
            int nc = cur.col + dx[i], nr = cur.row + dy[i];
            if (grid_.isBlocked(nc, nr)) continue;
            int nk = key(nc, nr);

            // Theta*: try to connect through parent
            int pk = parent[curKey];
            int pc = pk % Config::GRID_COLS;
            int pr = pk / Config::GRID_COLS;

            float ng;
            int newParent;
            if (lineOfSight(pc, pr, nc, nr)) {
                ng = gScore[pk] + heuristic(pc, pr, nc, nr);
                newParent = pk;
            } else {
                ng = gScore[curKey] + heuristic(cur.col, cur.row, nc, nr);
                newParent = curKey;
            }

            if (gScore.find(nk) == gScore.end() || ng < gScore[nk]) {
                gScore[nk] = ng;
                parent[nk] = newParent;
                cameFrom[nk] = newParent;
                open.push({nc, nr, ng, ng + heuristic(nc, nr, ec, er)});
            }
        }
    }

    if (!found) return {end};

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
    if (!path.empty()) path.back() = end;
    else path.push_back(end);
    return path;
}