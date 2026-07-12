#include "NavMesh.h"
#include "../Map/Map.h"
#include "../Utils/Math.h"
#include "../Graphics/Renderer.h"
#include <algorithm>
#include <queue>
#include <set>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

//NavNode::NavNode() : id(-1) {}

// NavNode methods
bool NavNode::Contains(const Vector2& point) const {
    if (vertices.empty()) return false;
    
    // Ray casting algorithm for point-in-polygon
    int n = vertices.size();
    bool inside = false;
    
    for (int i = 0, j = n - 1; i < n; j = i++) {
        float xi = vertices[i].x;
        float yi = vertices[i].y;
        float xj = vertices[j].x;
        float yj = vertices[j].y;
        
        if (((yi > point.y) != (yj > point.y)) &&
            (point.x < (xj - xi) * (point.y - yi) / (yj - yi) + xi)) {
            inside = !inside;
        }
    }
    
    return inside;
}

Point2D NavNode::GetClosestPointOnEdge(const Vector2& point) const {
    if (vertices.empty()) return center;
    
    Point2D closest = center;
    float minDist = 999999.0f;
    
    int n = vertices.size();
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        
        Vector2 a(vertices[i].x, vertices[i].y);
        Vector2 b(vertices[j].x, vertices[j].y);
        
        Vector2 ab = b - a;
        Vector2 ap = point - a;
        
        float abLengthSq = ab.x * ab.x + ab.y * ab.y;
        if (abLengthSq < 0.001f) continue;
        
        float t = (ap.x * ab.x + ap.y * ab.y) / abLengthSq;
        t = std::max(0.0f, std::min(1.0f, t));
        
        Vector2 closestOnEdge = a + ab * t;
        float dist = point.Distance(closestOnEdge);
        
        if (dist < minDist) {
            minDist = dist;
            closest = Point2D((int)closestOnEdge.x, (int)closestOnEdge.y);
        }
    }
    
    return closest;
}

// NavPath methods
Vector2 NavPath::GetCurrentWaypoint() const {
    if (waypoints.empty()) return Vector2(0, 0);
    if (currentWaypointIndex >= waypoints.size()) return waypoints.back();
    return waypoints[currentWaypointIndex];
}

void NavPath::AdvanceWaypoint() {
    if (currentWaypointIndex < waypoints.size()) {
        currentWaypointIndex++;
    }
}

bool NavPath::IsComplete() const {
    return currentWaypointIndex >= waypoints.size();
}

// NavMesh methods
NavMesh::NavMesh(Map* map) : map(map) {
}

void NavMesh::Build() {
    if (!map) return;
    
    nodes.clear();
    
    int gridWidth = map->GetWidth() * GRID_RESOLUTION;
    int gridHeight = map->GetHeight() * GRID_RESOLUTION;
    
    nodeGrid.clear();
    nodeGrid.resize(gridWidth);
    for (int i = 0; i < gridWidth; i++) {
        nodeGrid[i].resize(gridHeight, -1);
    }
    
    GenerateNodes();
    ConnectNeighbors();
}

int NavMesh::GetNodeAt(const Vector2& position) const {
    if (!map) return -1;
    
    int gx = (int)(position.x / 32.0f * GRID_RESOLUTION);
    int gy = (int)(position.y / 32.0f * GRID_RESOLUTION);
    
    int gridWidth = map->GetWidth() * GRID_RESOLUTION;
    int gridHeight = map->GetHeight() * GRID_RESOLUTION;
    
    // 1. Fast direct lookup
    if (gx >= 0 && gx < gridWidth && gy >= 0 && gy < gridHeight) {
        int nodeId = nodeGrid[gx][gy];
        if (nodeId >= 0 && nodeId < (int)nodes.size()) {
            return nodeId;
        }
    }
    
    // 2. Local fallback: only check immediate adjacent grid cells instead of ALL 200,000 nodes
    for (int dy = -1; dy <= 1; dy++) {
        for (int dx = -1; dx <= 1; dx++) {
            int checkGx = gx + dx;
            int checkGy = gy + dy;
            if (checkGx >= 0 && checkGx < gridWidth && checkGy >= 0 && checkGy < gridHeight) {
                int nodeId = nodeGrid[checkGx][checkGy];
                if (nodeId >= 0 && nodeId < (int)nodes.size() && nodes[nodeId].Contains(position)) {
                    return nodeId;
                }
            }
        }
    }
    
    return -1;
}

const NavNode* NavMesh::GetNode(int nodeId) const {
    if (nodeId < 0 || nodeId >= nodes.size()) return nullptr;
    return &nodes[nodeId];
}

std::vector<int> NavMesh::GetNodesInRadius(const Vector2& position, float radius) const {
    std::vector<int> result;
    std::unordered_set<int> visited;
    
    int startNode = GetNodeAt(position);
    if (startNode < 0) return result;
    
    std::queue<int> toCheck;
    toCheck.push(startNode);
    visited.insert(startNode);
    
    while (!toCheck.empty()) {
        int currentId = toCheck.front();
        toCheck.pop();
        
        if (currentId < 0 || currentId >= nodes.size()) continue;
        
        const NavNode& current = nodes[currentId];
        
        // Check if this node is within radius
        float distToCenter = position.Distance(
            Vector2(current.center.x, current.center.y));
        
        if (distToCenter <= radius) {
            result.push_back(currentId);
            
            for (int neighborId : current.neighbors) {
                if (visited.find(neighborId) == visited.end()) {
                    visited.insert(neighborId);
                    toCheck.push(neighborId);
                }
            }
        }
    }
    
    return result;
}

NavPath NavMesh::FindPath(const Vector2& start, const Vector2& end) const {
    NavPath result;
    
    int startNode = GetNodeAt(start);
    int endNode = GetNodeAt(end);
    
    if (startNode < 0 || endNode < 0) {
        // Can't find path - return empty
        return result;
    }
    
    if (startNode == endNode) {
        result.nodeIds = {startNode};
        result.waypoints = {end};
        result.currentWaypointIndex = 0;
        return result;
    }
    
    // A* on NavMesh nodes
    std::unordered_map<int, float> gCost;
    std::unordered_map<int, float> fCost;
    std::unordered_map<int, int> parent;
    std::unordered_set<int> closed;
    
    auto compare = [&fCost](int a, int b) {
        float fa = fCost.count(a) ? fCost[a] : std::numeric_limits<float>::max();
        float fb = fCost.count(b) ? fCost[b] : std::numeric_limits<float>::max();
        return fa > fb;
    };
    
    std::priority_queue<int, std::vector<int>, decltype(compare)> open(compare);
    
    gCost[startNode] = 0;
    fCost[startNode] = Heuristic(startNode, endNode);
    open.push(startNode);
    
    int iterations = 0;
    const int MAX_ITERATIONS = 5000;
    
    while (!open.empty() && iterations < MAX_ITERATIONS) {
        iterations++;
        
        int current = open.top();
        open.pop();
        
        if (current == endNode) {
            // Reconstruct path
            std::vector<int> path;
            int node = endNode;
            while (node != startNode) {
                path.push_back(node);
                if (parent.find(node) == parent.end()) break;
                node = parent[node];
            }
            path.push_back(startNode);
            std::reverse(path.begin(), path.end());
            
            result.nodeIds = path;
            result.waypoints = SmoothPath(path, start, end);
            result.currentWaypointIndex = 0;
            return result;
        }
        
        if (closed.find(current) != closed.end()) continue;
        closed.insert(current);
        
        if (current < 0 || current >= nodes.size()) continue;
        
        const NavNode& node = nodes[current];
        for (int neighborId : node.neighbors) {
            if (closed.find(neighborId) != closed.end()) continue;
            if (neighborId < 0 || neighborId >= nodes.size()) continue;
            
            float edgeCost = Vector2(
                nodes[current].center.x, nodes[current].center.y
            ).Distance(Vector2(
                nodes[neighborId].center.x, nodes[neighborId].center.y
            ));
            
            float newGCost = gCost[current] + edgeCost;
            
            if (!gCost.count(neighborId) || newGCost < gCost[neighborId]) {
                gCost[neighborId] = newGCost;
                fCost[neighborId] = newGCost + Heuristic(neighborId, endNode);
                parent[neighborId] = current;
                open.push(neighborId);
            }
        }
    }
    
    return result; // No path found
}

Vector2 NavMesh::GetRandomPoint() const {
    if (nodes.empty()) return Vector2(0, 0);
    
    const NavNode& node = nodes[rand() % nodes.size()];
    return Vector2(node.center.x, node.center.y);
}

void NavMesh::GenerateNodes() {
    if (!map) return;
    
    nodes.clear();
    
    // Create nodes for walkable areas
    int nodeId = 0;
    for (int y = 0; y < map->GetHeight(); y++) {
        for (int x = 0; x < map->GetWidth(); x++) {
            if (map->IsWalkable(x, y) && !map->IsTileBlockedByStaticEntity(x, y, -1)) {
                // Create a NavNode for this tile
                NavNode node(nodeId);
                
                float worldX = x * 32.0f;
                float worldY = y * 32.0f;
                
                node.center = Point2D(worldX + 16, worldY + 16);
                node.vertices = {
                    Point2D(worldX, worldY),
                    Point2D(worldX + 32, worldY),
                    Point2D(worldX + 32, worldY + 32),
                    Point2D(worldX, worldY + 32)
                };
                
                // Mark grid cells
                int gridWidth = map->GetWidth() * GRID_RESOLUTION;
                int gridHeight = map->GetHeight() * GRID_RESOLUTION;
                
                for (int gy = 0; gy < GRID_RESOLUTION; gy++) {
                    for (int gx = 0; gx < GRID_RESOLUTION; gx++) {
                        int gridX = x * GRID_RESOLUTION + gx;
                        int gridY = y * GRID_RESOLUTION + gy;
                        if (gridX >= 0 && gridX < gridWidth && 
                            gridY >= 0 && gridY < gridHeight) {
                            nodeGrid[gridX][gridY] = nodeId;
                        }
                    }
                }
                
                nodes.push_back(node);
                nodeId++;
            }
        }
    }
}

void NavMesh::SimplifyNodes() {
    // Simplified version - for production you'd use convex decomposition
    // For now, we keep tile-based nodes
}

void NavMesh::ConnectNeighbors() {
    // 8 directions: 4 cardinal + 4 diagonal
    const int dx[] = {-1, 0, 1, -1, 1, -1, 0, 1};
    const int dy[] = {-1, -1, -1, 0, 0, 1, 1, 1};
    
    int gridWidth = map->GetWidth() * GRID_RESOLUTION;
    int gridHeight = map->GetHeight() * GRID_RESOLUTION;

    // O(N) Loop: only check the 8 adjacent tiles instead of all 200,000 nodes!
    for (size_t i = 0; i < nodes.size(); i++) {
        Point2D center = nodes[i].center;
        int tileX = (int)(center.x / 32.0f);
        int tileY = (int)(center.y / 32.0f);

        for (int dir = 0; dir < 8; dir++) {
            int nx = tileX + dx[dir];
            int ny = tileY + dy[dir];

            // Check map bounds
            if (nx < 0 || nx >= map->GetWidth() || ny < 0 || ny >= map->GetHeight()) {
                continue;
            }

            // Look up the node ID at the center of the adjacent tile directly from nodeGrid
            int gridX = nx * GRID_RESOLUTION + (GRID_RESOLUTION / 2);
            int gridY = ny * GRID_RESOLUTION + (GRID_RESOLUTION / 2);

            if (gridX >= 0 && gridX < gridWidth && gridY >= 0 && gridY < gridHeight) {
                int neighborId = nodeGrid[gridX][gridY];

                // If a valid neighbor node exists there
                if (neighborId >= 0 && neighborId != (int)i) {
                    // Prevent diagonal corner-cutting through obstacles
                    if (dir >= 4) { // Diagonal check
                        int checkX1 = tileX + dx[dir];
                        int checkY1 = tileY;
                        int checkX2 = tileX;
                        int checkY2 = tileY + dy[dir];

                        int gX1 = checkX1 * GRID_RESOLUTION + (GRID_RESOLUTION / 2);
                        int gY1 = checkY1 * GRID_RESOLUTION + (GRID_RESOLUTION / 2);
                        int gX2 = checkX2 * GRID_RESOLUTION + (GRID_RESOLUTION / 2);
                        int gY2 = checkY2 * GRID_RESOLUTION + (GRID_RESOLUTION / 2);

                        bool clear1 = (gX1 >= 0 && gX1 < gridWidth && gY1 >= 0 && gY1 < gridHeight && nodeGrid[gX1][gY1] >= 0);
                        bool clear2 = (gX2 >= 0 && gX2 < gridWidth && gY2 >= 0 && gY2 < gridHeight && nodeGrid[gX2][gY2] >= 0);

                        if (!clear1 || !clear2) continue; // Block diagonal if corner is solid
                    }

                    nodes[i].neighbors.push_back(neighborId);
                }
            }
        }
    }
}


bool NavMesh::AreNodesAdjacent(const NavNode& a, const NavNode& b) const {
    // Two nodes are adjacent if their centers are within ~1-2 tiles
    int tileSize = 32;
    int dx = std::abs(a.center.x - b.center.x);
    int dy = std::abs(a.center.y - b.center.y);
    
    // Adjacent tiles (horizontal, vertical, or diagonal)
    return (dx == tileSize && dy == 0) || 
           (dx == 0 && dy == tileSize) ||
           (dx == tileSize && dy == tileSize);
}

float NavMesh::Heuristic(int nodeA, int nodeB) const {
    if (nodeA < 0 || nodeA >= nodes.size() || 
        nodeB < 0 || nodeB >= nodes.size()) {
        return std::numeric_limits<float>::max();
    }
    
    int dx = nodes[nodeA].center.x - nodes[nodeB].center.x;
    int dy = nodes[nodeA].center.y - nodes[nodeB].center.y;
    return std::sqrt(dx * dx + dy * dy);
}

std::vector<Vector2> NavMesh::SmoothPath(const std::vector<int>& nodePath,
                                         const Vector2& start, 
                                         const Vector2& end) const {
    std::vector<Vector2> waypoints;
    waypoints.push_back(start);
    
    if (nodePath.size() > 2) {
        // Add intermediate waypoints (node centers)
        for (size_t i = 1; i < nodePath.size() - 1; i++) {
            int nodeId = nodePath[i];
            if (nodeId >= 0 && nodeId < nodes.size()) {
                Vector2 center(nodes[nodeId].center.x, nodes[nodeId].center.y);
                waypoints.push_back(center);
            }
        }
    }
    
    waypoints.push_back(end);
    return waypoints;
}

void NavMesh::DebugRender(Renderer* renderer) const {
    if (!renderer) return;
    
    // Draw navmesh nodes
    for (const NavNode& node : nodes) {
        // Draw node polygon
        for (size_t i = 0; i < node.vertices.size(); i++) {
            size_t j = (i + 1) % node.vertices.size();
            Vector2 a(node.vertices[i].x, node.vertices[i].y);
            Vector2 b(node.vertices[j].x, node.vertices[j].y);
            renderer->DrawLine(a, b, glm::vec3(0.0f, 0.5f, 0.0f), 1.0f);
        }
    }
}