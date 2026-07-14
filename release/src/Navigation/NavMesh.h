#ifndef NAVMESH_H
#define NAVMESH_H

#include "../Utils/Math.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

struct NavPolygon {
    int id;
    std::vector<Vector2> vertices; // Convex polygon vertices (world coords)
    Vector2 center;
    std::vector<int> neighbors;    // Adjacent polygon IDs
    
    NavPolygon() : id(-1), center(0, 0) {}
    NavPolygon(int polyId) : id(polyId), center(0, 0) {}
    
    bool Contains(const Vector2& point) const;
    Vector2 GetCenter() const { return center; }
    void CalculateCenter();
    float GetArea() const;
};

struct NavPath {
    std::vector<int> polygonIds;   // NavMesh polygons on the path
    std::vector<Vector2> waypoints; // Smoothed world positions
    int currentWaypointIndex;
    
    NavPath() : currentWaypointIndex(0) {}
    
    Vector2 GetCurrentWaypoint() const;
    bool HasNextWaypoint() const;
    void AdvanceWaypoint();
    bool IsComplete() const;
};

class NavMesh {
public:
    NavMesh(class Map* map);
    ~NavMesh() = default;
    
    void Build();
    
    // Query
    int GetPolygonAt(const Vector2& position) const;
    const NavPolygon* GetPolygon(int id) const;
    
    // Pathfinding
    NavPath FindPath(const Vector2& start, const Vector2& end) const;
    
    // Validation
    bool IsPointWalkable(const Vector2& point) const;
    
    // Debug
    void DebugRender(class Renderer* renderer) const;
    
private:
    class Map* map;
    std::vector<NavPolygon> polygons;
    
    // Build phase
    void BuildPolygons();
    void MergeAdjacentRectangles();
    void ConnectNeighbors();
    
    // Pathfinding
    float Heuristic(int polyA, int polyB) const;
    std::vector<Vector2> FunnelAlgorithm(const std::vector<int>& polygonPath,
                                         const Vector2& start,
                                         const Vector2& end) const;
    
    // Find best crossing point on a portal segment
    Vector2 ClosestPointOnSegment(const Vector2& from, const Vector2& to,
                                  const Vector2& segA, const Vector2& segB) const;
    
    // Remove unnecessary waypoints
    std::vector<Vector2> OptimizePath(const std::vector<Vector2>& path) const;
    
    // Check if straight line is walkable
    bool HasLineOfSight(const Vector2& from, const Vector2& to) const;
    
    struct Edge {
        Vector2 a, b;
        Edge(const Vector2& pa, const Vector2& pb) : a(pa), b(pb) {}
    };
    
    Edge GetSharedEdge(int poly1, int poly2) const;
    bool PolygonsShareEdge(int poly1, int poly2) const;
    
    // Spatial lookup (for GetPolygonAt)
    std::vector<std::vector<int>> spatialGrid;
    int gridWidth, gridHeight;
    static const int GRID_CELL_SIZE = 32; // One tile

    // Merging rectangles helpers
    bool CanMergePolygons(int idx1, int idx2) const;
    std::vector<Vector2> MergeVertices(const NavPolygon& p1, const NavPolygon& p2) const;
    bool IsConvex(const std::vector<Vector2>& vertices) const;
    Rect GetPolygonBoundingBox(const NavPolygon& poly) const;
    void MergeTwoPolygons(int idx1, int idx2);
    void UpdateSpatialGridForPolygon(int polyId, const NavPolygon& poly);
    void ClearSpatialGridForPolygon(int polyId);
    void CompactPolygons();

    // Triangle signed area * 2 (used by funnel algorithm)
    static float TriangleArea2(const Vector2& a, const Vector2& b, const Vector2& c);
};

#endif // NAVMESH_H