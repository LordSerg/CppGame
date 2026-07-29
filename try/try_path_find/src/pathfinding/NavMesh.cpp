#include "pathfinding/NavMesh.h"
#include "simulation/World.h"
#include "core/Config.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>
#include <iostream>

Vec2 NavTriangle::centroid() const {
    return Vec2((vertices[0].x + vertices[1].x + vertices[2].x) / 3.0f,
                (vertices[0].y + vertices[1].y + vertices[2].y) / 3.0f);
}

static float cross2D(Vec2 a, Vec2 b) {
    return a.x * b.y - a.y * b.x;
}

static float triArea2D(Vec2 a, Vec2 b, Vec2 c) {
    return (c.x - a.x) * (b.y - a.y) - (b.x - a.x) * (c.y - a.y);
}

static bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    float d1 = cross2D(b - a, p - a);
    float d2 = cross2D(c - b, p - b);
    float d3 = cross2D(a - c, p - c);
    bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(hasNeg && hasPos);
}

// Check if two line segments share an edge exactly
static bool edgesMatch(Vec2 a1, Vec2 a2, Vec2 b1, Vec2 b2) {
    auto eq = [](Vec2 v1, Vec2 v2) {
        return std::abs(v1.x - v2.x) < 1e-3f && std::abs(v1.y - v2.y) < 1e-3f;
    };
    return (eq(a1, b1) && eq(a2, b2)) || (eq(a1, b2) && eq(a2, b1));
}

void NavMesh::rebuild(const World& world) {
    triangles_.clear();

    std::vector<bool> blocked(Config::GRID_COLS * Config::GRID_ROWS, false);
    for (const auto& b : world.getBarriers()) {
        int minC = std::max(0, (int)((b.center.x - b.halfExtents.x) / Config::CELL_WIDTH));
        int minR = std::max(0, (int)((b.center.y - b.halfExtents.y) / Config::CELL_HEIGHT));
        int maxC = std::min(Config::GRID_COLS - 1, (int)((b.center.x + b.halfExtents.x) / Config::CELL_WIDTH));
        int maxR = std::min(Config::GRID_ROWS - 1, (int)((b.center.y + b.halfExtents.y) / Config::CELL_HEIGHT));
        for (int r = minR; r <= maxR; ++r)
            for (int c = minC; c <= maxC; ++c)
                blocked[r * Config::GRID_COLS + c] = true;
    }

    // 1. Greedy Meshing: Merge walkable cells into large rectangles
    std::vector<bool> visited = blocked; 
    std::vector<NavTriangle> rawTris;

    for (int r = 0; r < Config::GRID_ROWS; ++r) {
        for (int c = 0; c < Config::GRID_COLS; ++c) {
            if (visited[r * Config::GRID_COLS + c]) continue;

            // Expand right
            int w = 1;
            while (c + w < Config::GRID_COLS && !visited[r * Config::GRID_COLS + c + w]) {
                w++;
            }

            // Expand down
            int h = 1;
            bool canExpand = true;
            while (r + h < Config::GRID_ROWS && canExpand) {
                for (int i = 0; i < w; ++i) {
                    if (visited[(r + h) * Config::GRID_COLS + c + i]) {
                        canExpand = false;
                        break;
                    }
                }
                if (canExpand) h++;
            }

            // Mark visited
            for (int rr = 0; rr < h; ++rr) {
                for (int cc = 0; cc < w; ++cc) {
                    visited[(r + rr) * Config::GRID_COLS + c + cc] = true;
                }
            }

            // Triangulate rectangle
            float x0 = c * Config::CELL_WIDTH;
            float y0 = r * Config::CELL_HEIGHT;
            float x1 = x0 + w * Config::CELL_WIDTH;
            float y1 = y0 + h * Config::CELL_HEIGHT;

            NavTriangle t1, t2;
            t1.vertices[0] = {x0, y0}; t1.vertices[1] = {x1, y0}; t1.vertices[2] = {x0, y1};
            t2.vertices[0] = {x1, y0}; t2.vertices[1] = {x1, y1}; t2.vertices[2] = {x0, y1};
            rawTris.push_back(t1);
            rawTris.push_back(t2);
        }
    }
    triangles_ = rawTris;

    // 2. Link Neighbors (Portal Graph)
    for (size_t i = 0; i < triangles_.size(); ++i) {
        for (int e1 = 0; e1 < 3; ++e1) {
            Vec2 v1a = triangles_[i].vertices[e1];
            Vec2 v1b = triangles_[i].vertices[(e1 + 1) % 3];

            for (size_t j = i + 1; j < triangles_.size(); ++j) {
                for (int e2 = 0; e2 < 3; ++e2) {
                    Vec2 v2a = triangles_[j].vertices[e2];
                    Vec2 v2b = triangles_[j].vertices[(e2 + 1) % 3];

                    if (edgesMatch(v1a, v1b, v2a, v2b)) {
                        triangles_[i].neighbors[e1] = (int)j;
                        triangles_[j].neighbors[e2] = (int)i;
                    }
                }
            }
        }
    }
}

int NavMesh::findTriangle(Vec2 point) const {
    for (size_t i = 0; i < triangles_.size(); ++i) {
        if (pointInTriangle(point, triangles_[i].vertices[0], triangles_[i].vertices[1], triangles_[i].vertices[2]))
            return (int)i;
    }
    // Fallback: closest centroid
    int best = -1;
    float bestD = 1e9f;
    for (size_t i = 0; i < triangles_.size(); ++i) {
        float d = (triangles_[i].centroid() - point).lengthSq();
        if (d < bestD) { bestD = d; best = (int)i; }
    }
    return best;
}

std::vector<Vec2> NavMesh::findPath(Vec2 start, Vec2 end) const {
    if (triangles_.empty()) return {end};

    int startIdx = findTriangle(start);
    int endIdx = findTriangle(end);
    if (startIdx == -1 || endIdx == -1) return {end};

    // 1. A* over triangles
    struct Node { int id; float g, f; bool operator>(const Node& o) const { return f > o.f; } };
    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> open;
    std::unordered_map<int, float> gScore;
    std::unordered_map<int, int> cameFrom;

    open.push({startIdx, 0.0f, (triangles_[startIdx].centroid() - end).length()});
    gScore[startIdx] = 0.0f;

    bool found = false;
    while (!open.empty()) {
        int cur = open.top().id; open.pop();
        if (cur == endIdx) { found = true; break; }

        for (int i = 0; i < 3; ++i) {
            int nxt = triangles_[cur].neighbors[i];
            if (nxt == -1) continue;
            float ng = gScore[cur] + (triangles_[cur].centroid() - triangles_[nxt].centroid()).length();
            if (gScore.find(nxt) == gScore.end() || ng < gScore[nxt]) {
                gScore[nxt] = ng;
                cameFrom[nxt] = cur;
                open.push({nxt, ng, ng + (triangles_[nxt].centroid() - end).length()});
            }
        }
    }

    if (!found) return {end};

    // Reconstruct channel
    std::vector<int> channel;
    int curr = endIdx;
    while (curr != startIdx) {
        channel.push_back(curr);
        curr = cameFrom[curr];
    }
    channel.push_back(startIdx);
    std::reverse(channel.begin(), channel.end());

    // 2. Simple Funnel Algorithm
    std::vector<Vec2> leftPortals, rightPortals;
    leftPortals.push_back(start);
    rightPortals.push_back(start);

    for (size_t i = 0; i < channel.size() - 1; ++i) {
        const auto& t1 = triangles_[channel[i]];
        const auto& t2 = triangles_[channel[i+1]];
        // Find shared edge
        for (int e = 0; e < 3; ++e) {
            if (t1.neighbors[e] == channel[i+1]) {
                Vec2 p1 = t1.vertices[e];
                Vec2 p2 = t1.vertices[(e + 1) % 3];
                // Order to Left and Right based on traversal direction
                if (triArea2D(t1.centroid(), p1, p2) > 0.0f) {
                    leftPortals.push_back(p1); rightPortals.push_back(p2);
                } else {
                    leftPortals.push_back(p2); rightPortals.push_back(p1);
                }
                break;
            }
        }
    }
    leftPortals.push_back(end);
    rightPortals.push_back(end);

    std::vector<Vec2> path;
    path.push_back(start);

    Vec2 portalApex = start;
    Vec2 portalLeft = portalApex, portalRight = portalApex;
    int apexIndex = 0, leftIndex = 0, rightIndex = 0;

    for (int i = 1; i < (int)leftPortals.size(); ++i) {
        Vec2 left = leftPortals[i], right = rightPortals[i];

        // Update right
        if (triArea2D(portalApex, portalRight, right) <= 0.0f) {
            if (portalApex.x == portalRight.x && portalApex.y == portalRight.y || triArea2D(portalApex, portalLeft, right) > 0.0f) {
                portalRight = right;
                rightIndex = i;
            } else {
                path.push_back(portalLeft);
                portalApex = portalLeft;
                portalRight = portalApex;
                apexIndex = leftIndex;
                i = apexIndex;
                continue;
            }
        }

        // Update left
        if (triArea2D(portalApex, portalLeft, left) >= 0.0f) {
            if (portalApex.x == portalLeft.x && portalApex.y == portalLeft.y || triArea2D(portalApex, portalRight, left) < 0.0f) {
                portalLeft = left;
                leftIndex = i;
            } else {
                path.push_back(portalRight);
                portalApex = portalRight;
                portalLeft = portalApex;
                apexIndex = rightIndex;
                i = apexIndex;
                continue;
            }
        }
    }
    path.push_back(end);
    return path;
}