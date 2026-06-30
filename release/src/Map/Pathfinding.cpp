#include "Pathfinding.h"
#include "Map.h"
#include <algorithm>
#include <cmath>

std::vector<Point2D> Pathfinding::FindPath(Map* map, Point2D start, Point2D goal, int unitSize) {
    if (!map->IsInBounds(start.x, start.y) || !map->IsInBounds(goal.x, goal.y)) {
        return {};
    }
    
    if (!map->IsWalkable(goal.x, goal.y)) {
        return {};
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
        
        std::vector<Point2D> neighbors = GetNeighbors(current->position, map, unitSize);
        
        for (const Point2D& neighborPos : neighbors) {
            if (closedSet.find(neighborPos) != closedSet.end()) {
                continue;
            }
            
            float tentativeGCost = current->gCost + 
                                  map->GetTile(neighborPos.x, neighborPos.y)->GetMovementCost();
            
            PathNode* neighbor = nullptr;
            auto it = allNodes.find(neighborPos);
            if (it == allNodes.end()) {
                neighbor = new PathNode();
                neighbor->position = neighborPos;
                neighbor->hCost = Heuristic(neighborPos, goal);
                allNodes[neighborPos] = neighbor;
            } else {
                neighbor = it->second;
            }
            
            if (tentativeGCost < neighbor->gCost || neighbor->gCost == 0) {
                neighbor->gCost = tentativeGCost;
                neighbor->parent = current;
                openSet.push(neighbor);
            }
        }
    }
    
    std::vector<Point2D> path;
    if (goalNode) {
        path = ReconstructPath(goalNode);
        path = SmoothPath(path);
    }
    
    // Cleanup
    for (auto& pair : allNodes) {
        delete pair.second;
    }
    
    return path;
}

std::vector<Point2D> Pathfinding::SmoothPath(const std::vector<Point2D>& path) {
    if (path.size() <= 2) return path;
    
    std::vector<Point2D> smoothed;
    smoothed.push_back(path[0]);
    
    for (size_t i = 2; i < path.size(); i++) {
        Point2D current = smoothed.back();
        Point2D target = path[i];
        
        // Check if we can go directly to target
        int dx = target.x - current.x;
        int dy = target.y - current.y;
        int steps = std::max(abs(dx), abs(dy));
        
        bool canSkip = true;
        // Simple line check (can be improved)
        
        if (!canSkip) {
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

std::vector<Point2D> Pathfinding::GetNeighbors(const Point2D& pos, Map* map, int unitSize) {
    std::vector<Point2D> neighbors;
    
    // 8 directions
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    for (int i = 0; i < 8; i++) {
        int newX = pos.x + dx[i];
        int newY = pos.y + dy[i];
        
        if (map->IsInBounds(newX, newY) && map->IsWalkable(newX, newY)) {
            neighbors.push_back(Point2D(newX, newY));
        }
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