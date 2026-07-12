#ifndef NAVMESH_H
#define NAVMESH_H

#include "../Utils/Math.h"
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

struct NavNode {
    int id;
    Point2D center;
    std::vector<Point2D> vertices; // Polygon vertices (convex polygon)
    std::vector<int> neighbors;    // Adjacent node IDs

    NavNode() : id(-1), center(0, 0) {}
    NavNode(int nodeId) : id(nodeId), center(0, 0) {}
    
    Point2D GetCenter() const { return center; }
    bool Contains(const Vector2& point) const;
    Point2D GetClosestPointOnEdge(const Vector2& point) const;
};

struct NavPath {
    std::vector<int> nodeIds;      // NavMesh nodes on the path
    std::vector<Vector2> waypoints; // Actual world positions
    int currentWaypointIndex;

    NavPath() : currentWaypointIndex(0) {}
    
    Vector2 GetCurrentWaypoint() const;
    void AdvanceWaypoint();
    bool IsComplete() const;
};

class NavMesh {
public:
    //NavMesh();
    NavMesh(class Map* map);
    ~NavMesh() = default;
    
    // Build NavMesh from map data
    void Build();
    
    // Query NavMesh
    int GetNodeAt(const Vector2& position) const;
    const NavNode* GetNode(int nodeId) const;
    std::vector<int> GetNodesInRadius(const Vector2& position, float radius) const;
    
    // Pathfinding on NavMesh
    NavPath FindPath(const Vector2& start, const Vector2& end) const;
    
    // Get random point on NavMesh (for wandering)
    Vector2 GetRandomPoint() const;
    
    // Debug rendering
    void DebugRender(class Renderer* renderer) const;
    
private:
    class Map* map;
    std::vector<NavNode> nodes;
    
    // Build helper methods
    void GenerateNodes();
    void ConnectNeighbors();
    void SimplifyNodes();
    bool AreNodesAdjacent(const NavNode& a, const NavNode& b) const;
    
    // A* on NavMesh
    float Heuristic(int nodeA, int nodeB) const;
    std::vector<Vector2> SmoothPath(const std::vector<int>& nodePath, 
                                    const Vector2& start, const Vector2& end) const;
    
    // Grid-based creation
    std::vector<std::vector<int>> nodeGrid; // Quick lookup: grid[x][y] -> nodeId
    
    static const int GRID_RESOLUTION = 4; // NavMesh nodes per tile
};

#endif // NAVMESH_H