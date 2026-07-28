#include "pathfinding/NavMesh.h"
#include "simulation/World.h"
#include "core/Config.h"
#include <algorithm>
#include <cmath>

Vec2 NavTriangle::centroid() const {
    return Vec2((vertices[0].x + vertices[1].x + vertices[2].x) / 3.0f,
                (vertices[0].y + vertices[1].y + vertices[2].y) / 3.0f);
}

void NavMesh::rebuild(const World& world) {
    // Simple grid-based triangulation: split each non-blocked cell into 2 triangles
    // This is a placeholder; a proper Delaunay or constrained triangulation
    // can be substituted later.
    triangles_.clear();

    // manually inline grid rebuild
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

    for (int r = 0; r < Config::GRID_ROWS; ++r) {
        for (int c = 0; c < Config::GRID_COLS; ++c) {
            if (blocked[r * Config::GRID_COLS + c]) continue;

            float x0 = c * Config::CELL_WIDTH;
            float y0 = r * Config::CELL_HEIGHT;
            float x1 = x0 + Config::CELL_WIDTH;
            float y1 = y0 + Config::CELL_HEIGHT;

            NavTriangle t1;
            t1.vertices[0] = {x0, y0};
            t1.vertices[1] = {x1, y0};
            t1.vertices[2] = {x0, y1};
            triangles_.push_back(t1);

            NavTriangle t2;
            t2.vertices[0] = {x1, y0};
            t2.vertices[1] = {x1, y1};
            t2.vertices[2] = {x0, y1};
            triangles_.push_back(t2);
        }
    }

    // Note: neighbor connectivity is not filled in this placeholder.
    // A real implementation would compute shared edges.
}

static float cross2D(Vec2 a, Vec2 b) {
    return a.x * b.y - a.y * b.x;
}

static bool pointInTriangle(Vec2 p, Vec2 a, Vec2 b, Vec2 c) {
    float d1 = cross2D(b - a, p - a);
    float d2 = cross2D(c - b, p - b);
    float d3 = cross2D(a - c, p - c);
    bool hasNeg = (d1 < 0) || (d2 < 0) || (d3 < 0);
    bool hasPos = (d1 > 0) || (d2 > 0) || (d3 > 0);
    return !(hasNeg && hasPos);
}

int NavMesh::findTriangle(Vec2 point) const {
    for (size_t i = 0; i < triangles_.size(); ++i) {
        if (pointInTriangle(point,
                            triangles_[i].vertices[0],
                            triangles_[i].vertices[1],
                            triangles_[i].vertices[2]))
            return (int)i;
    }
    return -1;
}

std::vector<Vec2> NavMesh::findPath(Vec2 start, Vec2 end) const {
    // Placeholder: direct path (proper implementation needs A* on triangle graph + funnel)
    return {end};
}