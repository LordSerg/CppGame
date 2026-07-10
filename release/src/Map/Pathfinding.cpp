#include "Pathfinding.h"
#include "Map.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>

std::vector<Point2D> Pathfinding::FindPath(Map* map, Point2D start, Point2D goal, 
                                           int unitSize, int excludeUnitId) {
    if (!map || !map->IsInBounds(start.x, start.y) || !map->IsInBounds(goal.x, goal.y)) {
        return {};
    }
    
    // If goal is unwalkable, find nearest walkable tile
    if (!map->IsWalkable(goal.x, goal.y)) {
        Point2D nearestWalkable = FindNearestWalkable(map, goal, 10);
        if (nearestWalkable.x < 0) {
            return {}; // No walkable tile found
        }
        goal = nearestWalkable;
    }
    
    // If start equals goal, return single-point path
    if (start == goal) {
        return {goal};
    }
    
    // A* pathfinding
    std::unordered_map<Point2D, std::unique_ptr<PathNode>> allNodes;
    std::priority_queue<PathNode*, std::vector<PathNode*>, PathNodeComparator> openSet;
    std::unordered_set<Point2D> closedSet;
    
    auto startNode = std::make_unique<PathNode>();
    startNode->position = start;
    startNode->gCost = 0;
    startNode->hCost = Heuristic(start, goal);
    startNode->parent = nullptr;
    
    PathNode* startPtr = startNode.get();
    allNodes[start] = std::move(startNode);
    openSet.push(startPtr);
    
    PathNode* goalNode = nullptr;
    int iterations = 0;
    const int maxIterations = 10000; // Prevent infinite loops
    
    while (!openSet.empty() && iterations < maxIterations) {
        iterations++;
        
        PathNode* current = openSet.top();
        openSet.pop();
        
        if (current->position == goal) {
            goalNode = current;
            break;
        }
        
        if (closedSet.find(current->position) != closedSet.end()) {
            continue;
        }
        
        closedSet.insert(current->position);
        
        std::vector<Point2D> neighbors = GetNeighbors(current->position, map, 
                                                     unitSize, excludeUnitId, goal);
        
        for (const Point2D& neighborPos : neighbors) {
            if (closedSet.find(neighborPos) != closedSet.end()) {
                continue;
            }
            
            // Calculate movement cost
            float baseCost = 1.0f;
            
            // Diagonal movement costs more
            if (neighborPos.x != current->position.x && 
                neighborPos.y != current->position.y) {
                baseCost = 1.414f; // sqrt(2)
            }
            
            // Add terrain cost
            const Tile* tile = map->GetTile(neighborPos.x, neighborPos.y);
            if (tile) {
                baseCost *= tile->GetMovementCost();
            }
            
            float tentativeGCost = current->gCost + baseCost;
            
            PathNode* neighbor = nullptr;
            auto it = allNodes.find(neighborPos);
            
            if (it == allNodes.end()) {
                auto newNode = std::make_unique<PathNode>();
                newNode->position = neighborPos;
                newNode->hCost = Heuristic(neighborPos, goal);
                newNode->gCost = std::numeric_limits<float>::max();
                neighbor = newNode.get();
                allNodes[neighborPos] = std::move(newNode);
            } else {
                neighbor = it->second.get();
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
    
    return path;
}

Point2D Pathfinding::FindNearestWalkable(Map* map, const Point2D& target, 
                                        int maxSearchRadius) {
    if (!map) return Point2D(-1, -1);
    
    // BFS to find nearest walkable tile
    std::queue<Point2D> queue;
    std::unordered_set<Point2D> visited;
    
    queue.push(target);
    visited.insert(target);
    
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    while (!queue.empty()) {
        Point2D current = queue.front();
        queue.pop();
        
        // Check if this tile is walkable
        if (map->IsInBounds(current.x, current.y) && 
            map->IsWalkable(current.x, current.y) &&
            !map->IsTileBlockedByStaticEntity(current.x, current.y, -1)) {
            return current;
        }
        
        // Don't search too far
        int distance = abs(current.x - target.x) + abs(current.y - target.y);
        if (distance >= maxSearchRadius) continue;
        
        // Add neighbors
        for (int i = 0; i < 8; i++) {
            Point2D next(current.x + dx[i], current.y + dy[i]);
            
            if (visited.find(next) == visited.end() && 
                map->IsInBounds(next.x, next.y)) {
                visited.insert(next);
                queue.push(next);
            }
        }
    }
    
    return Point2D(-1, -1);
}

bool Pathfinding::IsPathValid(const std::vector<Point2D>& path, Map* map, 
                              int excludeEntityId) {
    if (path.empty() || !map) return false;
    
    for (const Point2D& point : path) {
        if (!map->IsInBounds(point.x, point.y)) return false;
        if (!map->IsWalkable(point.x, point.y)) return false;
        
        // Check if a new static obstacle blocks this tile
        if (map->IsTileBlockedByStaticEntity(point.x, point.y, excludeEntityId)) {
            return false;
        }
    }
    
    return true;
}

std::vector<Point2D> Pathfinding::SmoothPath(const std::vector<Point2D>& path, 
                                            Map* map, int excludeEntityId) {
    if (path.size() <= 2) return path;
    
    std::vector<Point2D> smoothed;
    smoothed.push_back(path[0]);
    
    size_t currentIdx = 0;
    
    while (currentIdx < path.size() - 1) {
        size_t farthestVisible = currentIdx + 1;
        
        // Find the farthest point we can see from current position
        for (size_t i = currentIdx + 2; i < path.size(); i++) {
            if (HasLineOfSight(map, path[currentIdx], path[i], excludeEntityId)) {
                farthestVisible = i;
            } else {
                break;
            }
        }
        
        // Move to the farthest visible point
        smoothed.push_back(path[farthestVisible]);
        currentIdx = farthestVisible;
    }
    
    return smoothed;
}

bool Pathfinding::HasLineOfSight(Map* map, const Point2D& start, 
                                 const Point2D& end, int excludeEntityId) {
    if (!map) return false;
    
    // Bresenham's line algorithm
    int x0 = start.x, y0 = start.y;
    int x1 = end.x, y1 = end.y;
    
    int dx = abs(x1 - x0);
    int dy = abs(y1 - y0);
    int sx = (x0 < x1) ? 1 : -1;
    int sy = (y0 < y1) ? 1 : -1;
    int err = dx - dy;
    
    while (true) {
        // Check if current tile is blocked
        if (!map->IsInBounds(x0, y0)) return false;
        if (!map->IsWalkable(x0, y0)) return false;
        if (map->IsTileBlockedByStaticEntity(x0, y0, excludeEntityId)) return false;
        
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
    
    return true;
}

float Pathfinding::Heuristic(const Point2D& a, const Point2D& b) {
    // Euclidean distance for better diagonal movement
    int dx = abs(a.x - b.x);
    int dy = abs(a.y - b.y);
    return sqrt(dx * dx + dy * dy);
}

std::vector<Point2D> Pathfinding::GetNeighbors(const Point2D& pos, Map* map, 
                                               int unitSize, int excludeUnitId, 
                                               const Point2D& goal) {
    std::vector<Point2D> neighbors;
    
    // 8 directions
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    for (int i = 0; i < 8; i++) {
        int newX = pos.x + dx[i];
        int newY = pos.y + dy[i];
        
        if (!map->IsInBounds(newX, newY)) continue;
        if (!map->IsWalkable(newX, newY)) continue;
        
        // HARD BLOCK: Static obstacles (buildings, obstacles, trees, rocks)
        if (map->IsTileBlockedByStaticEntity(newX, newY, excludeUnitId)) continue;
        
        // For diagonal moves, check that we're not cutting corners
        if (i >= 4) { // Diagonal
            int checkX1 = pos.x + dx[i];
            int checkY1 = pos.y;
            int checkX2 = pos.x;
            int checkY2 = pos.y + dy[i];
            
            // Both adjacent tiles must be walkable
            if (!map->IsInBounds(checkX1, checkY1) || 
                !map->IsWalkable(checkX1, checkY1) ||
                map->IsTileBlockedByStaticEntity(checkX1, checkY1, excludeUnitId)) {
                continue;
            }
            
            if (!map->IsInBounds(checkX2, checkY2) || 
                !map->IsWalkable(checkX2, checkY2) ||
                map->IsTileBlockedByStaticEntity(checkX2, checkY2, excludeUnitId)) {
                continue;
            }
        }
        
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