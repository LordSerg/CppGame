#pragma once

#include "simulation/Agent.h"
#include <vector>
#include <cstdint>

class World;

struct NavTriangle {
    Vec2 vertices[3];
    int neighbors[3] = {-1, -1, -1}; // adjacent triangle indices
    Vec2 centroid() const;
};

class NavMesh {
public:
    void rebuild(const World& world);

    // Find which triangle contains a point
    int findTriangle(Vec2 point) const;

    // Simple funnel-based path through the nav mesh
    std::vector<Vec2> findPath(Vec2 start, Vec2 end) const;

    const std::vector<NavTriangle>& getTriangles() const { return triangles_; }
    bool isBuilt() const { return !triangles_.empty(); }

private:
    std::vector<NavTriangle> triangles_;
};