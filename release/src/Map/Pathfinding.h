#ifndef PATHFINDING_H
#define PATHFINDING_H

#include "../Utils/Math.h"
#include <vector>
#include <queue>
#include <unordered_map>

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
    static std::vector<Point2D> FindPath(Map* map, Point2D start, Point2D goal, int unitSize = 1, int excludeUnitId = -1);
    static std::vector<Point2D> SmoothPath(const std::vector<Point2D>& path);
    
private:
    static float Heuristic(const Point2D& a, const Point2D& b);
    static std::vector<Point2D> GetNeighbors(const Point2D& pos, Map* map, int unitSize, int excludeUnitId = -1, const Point2D& goal = Point2D(-1, -1));
    static std::vector<Point2D> ReconstructPath(PathNode* endNode);
};

// Hash function for Point2D to use in unordered_map
namespace std {
    template<>
    struct hash<Point2D> {
        size_t operator()(const Point2D& p) const {
            return hash<int>()(p.x) ^ (hash<int>()(p.y) << 1);
        }
    };
}

#endif // PATHFINDING_H