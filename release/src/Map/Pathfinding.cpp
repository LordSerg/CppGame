#include "Pathfinding.h"
#include "Map.h"
#include <algorithm>
#include <cmath>
#include <limits>

std::vector<Point2D> Pathfinding::FindPath(Map* map, Point2D start, Point2D goal, int unitSize, int excludeUnitId) {
    if (!map->IsInBounds(start.x, start.y) || !map->IsInBounds(goal.x, goal.y)) {
        return {};
    }
    
    if (!map->IsWalkable(goal.x, goal.y)) {
        return {};
    }
    
    // If start equals goal, no path needed
    if (start == goal) {
        return {goal};
    }
    
    std::unordered_map<Point2D, PathNode*> allNodes;
    std::priority_queue<PathNode*, std::vector<PathNode*>, PathNodeComparator> openSet;
    std::unordered_map<Point2D, bool> closedSet;
    
    PathNode* startNode = new PathNode();
    startNode->position = start;
    startNode->gCost = 0;
    startNode->hCost = Heuristic(start, goal);
    startNode->parent = nullptr;
    
    allNodes[start] = startNode;
    openSet.push(startNode);
    
    PathNode* goalNode = nullptr;
    
    while (!openSet.empty()) {
        PathNode* current = openSet.top();
        openSet.pop();
        
        if (current->position == goal) {
            goalNode = current;
            break;
        }
        
        closedSet[current->position] = true;
        
        std::vector<Point2D> neighbors = GetNeighbors(current->position, map, unitSize, excludeUnitId, goal);
        
        for (const Point2D& neighborPos : neighbors) {
            if (closedSet.find(neighborPos) != closedSet.end()) {
                continue;
            }
            
            float tileCost = map->GetTile(neighborPos.x, neighborPos.y)->GetMovementCost();
            
            // Add extra cost if a unit is on this tile (soft penalty, not hard block)
            if (map->IsTileOccupiedByUnit(neighborPos.x, neighborPos.y, excludeUnitId)) {
                tileCost += 10.0f; // High cost to discourage routing through units
            }
            
            float tentativeGCost = current->gCost + tileCost;
            
            PathNode* neighbor = nullptr;
            bool isNewNode = false;
            auto it = allNodes.find(neighborPos);
            if (it == allNodes.end()) {
                neighbor = new PathNode();
                neighbor->position = neighborPos;
                neighbor->hCost = Heuristic(neighborPos, goal);
                neighbor->gCost = std::numeric_limits<float>::max(); // unvisited
                allNodes[neighborPos] = neighbor;
                isNewNode = true;
            } else {
                neighbor = it->second;
            }
            
            if (tentativeGCost < neighbor->gCost) {
                neighbor->gCost = tentativeGCost;
                neighbor->parent = current;
                openSet.push(neighbor);
            }
        }
    }
    
    std::vector<Point2D> path;
    if (goalNode) {
        path = ReconstructPath(goalNode);
        path = SmoothPath(path, map, excludeUnitId);
    }
    
    // Cleanup
    for (auto& pair : allNodes) {
        delete pair.second;
    }
    
    return path;
}

std::vector<Point2D> Pathfinding::SmoothPath(const std::vector<Point2D>& path, Map* map, int excludeEntityId) {
    if (path.size() <= 2) return path;
    
    std::vector<Point2D> smoothed;
    smoothed.push_back(path[0]);
    
    for (size_t i = 2; i < path.size(); i++) {
        Point2D current = smoothed.back();
        Point2D target = path[i];
        
        // Check if we can go directly from current to target without crossing blocked tiles
        bool canSkip = true;
        
        if (map) {
            // Use Bresenham's line algorithm to check all tiles along the line
            int x0 = current.x;
            int y0 = current.y;
            int x1 = target.x;
            int y1 = target.y;
            
            int dx = abs(x1 - x0);
            int dy = abs(y1 - y0);
            int sx = (x0 < x1) ? 1 : -1;
            int sy = (y0 < y1) ? 1 : -1;
            int err = dx - dy;
            
            while (true) {
                // Check if this tile along the line is blocked by static barriers only
                // (units are handled by periodic repathing + collision avoidance)
                if (x0 != current.x || y0 != current.y) { // Don't check the start tile
                    if (!map->IsInBounds(x0, y0) || 
                        !map->IsWalkable(x0, y0) || 
                        map->IsTileBlockedByStaticEntity(x0, y0, excludeEntityId)) {
                        canSkip = false;
                        break;
                    }
                }
                
                if (x0 == x1 && y0 == y1) break;
                
                int e2 = 2 * err;
                if (e2 > -dy) {
                    err -= dy;
                    x0 += sx;
                }
                if (e2 < dx) {
                    err += dx;
                    y0 += sy;
                }
            }
        }
        
        if (!canSkip) {
            // Can't skip to target, add the intermediate waypoint
            smoothed.push_back(path[i - 1]);
        }
    }
    
    smoothed.push_back(path.back());
    return smoothed;
}

float Pathfinding::Heuristic(const Point2D& a, const Point2D& b) {
    // Manhattan distance for grid-based movement
    return abs(a.x - b.x) + abs(a.y - b.y);
}

std::vector<Point2D> Pathfinding::GetNeighbors(const Point2D& pos, Map* map, int unitSize, int excludeUnitId, const Point2D& goal) {
    std::vector<Point2D> neighbors;
    
    // 8 directions
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    for (int i = 0; i < 8; i++) {
        int newX = pos.x + dx[i];
        int newY = pos.y + dy[i];
        
        if (!map->IsInBounds(newX, newY)) continue;
        if (!map->IsWalkable(newX, newY)) continue;
        
        // Hard-block static obstacles (buildings, obstacles, trees, rocks, water)
        if (map->IsTileBlockedByStaticEntity(newX, newY, excludeUnitId)) continue;
        
        // Allow any tile - units are soft-penalized via high movement cost
        neighbors.push_back(Point2D(newX, newY));
    }
    
    return neighbors;
}

std::vector<Point2D> Pathfinding::ReconstructPath(PathNode* endNode) {
    std::vector<Point2D> path;
    PathNode* current = endNode;
    
    while (current != nullptr) {
        path.push_back(current->position);
        current = current->parent;
    }
    
    std::reverse(path.begin(), path.end());
    return path;
}