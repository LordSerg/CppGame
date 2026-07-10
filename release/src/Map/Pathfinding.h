#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "../Utils/Math.h"
#include <vector>
#include <queue>
#include <unordered_map>
#include <memory>
#include <unordered_set>

class Map;

struct PathNode {
    Point2D position;
    float gCost; // Distance from start
    float hCost; // Heuristic distance to goal
    float fCost() const { return gCost + hCost; }
    PathNode* parent;
    
    PathNode() : gCost(0), hCost(0), parent(nullptr) {}
};

struct PathNodeComparator {
    bool operator()(const PathNode* a, const PathNode* b) const {
        return a->fCost() > b->fCost();
    }
};

class Pathfinding {
public:
    // Main pathfinding function
    static std::vector<Point2D> FindPath(Map* map, Point2D start, Point2D goal, 
                                         int unitSize = 1, int excludeUnitId = -1);
    
    // Smooth path to reduce waypoints
    static std::vector<Point2D> SmoothPath(const std::vector<Point2D>& path, 
                                          Map* map, int excludeEntityId = -1);
    
    // Check if path is still valid (no new obstacles)
    static bool IsPathValid(const std::vector<Point2D>& path, Map* map, 
                           int excludeEntityId = -1);
    
    // Find nearest walkable tile to a target
    static Point2D FindNearestWalkable(Map* map, const Point2D& target, 
                                      int maxSearchRadius = 10);
    
private:
    static float Heuristic(const Point2D& a, const Point2D& b);
    static std::vector<Point2D> GetNeighbors(const Point2D& pos, Map* map, 
                                            int unitSize, int excludeUnitId, 
                                            const Point2D& goal);
    static std::vector<Point2D> ReconstructPath(PathNode* endNode);
    
    // Line of sight check for path smoothing
    static bool HasLineOfSight(Map* map, const Point2D& start, 
                              const Point2D& end, int excludeEntityId);
};

// Hash function for Point2D
namespace std {
    template<>
    struct hash<Point2D> {
        size_t operator()(const Point2D& p) const {
            return hash<int>()(p.x) ^ (hash<int>()(p.y) << 1);
        }
    };
}

#endif // PATHFINDING_H