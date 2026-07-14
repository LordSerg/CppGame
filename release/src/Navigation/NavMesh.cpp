#include "NavMesh.h"
#include "../Map/Map.h"
#include "../Utils/Math.h"
#include "../Graphics/Renderer.h"
#include <algorithm>
#include <queue>
#include <set>
#include <limits>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>

// NavPolygon implementation
bool NavPolygon::Contains(const Vector2& point) const {
    if (vertices.size() < 3) return false;
    
    // Point-in-polygon test (ray casting)
    bool inside = false;
    int n = vertices.size();
    
    for (int i = 0, j = n - 1; i < n; j = i++) {
        if (((vertices[i].y > point.y) != (vertices[j].y > point.y)) &&
            (point.x < (vertices[j].x - vertices[i].x) * (point.y - vertices[i].y) / 
                      (vertices[j].y - vertices[i].y) + vertices[i].x)) {
            inside = !inside;
        }
    }
    
    return inside;
}

void NavPolygon::CalculateCenter() {
    if (vertices.empty()) {
        center = Vector2(0, 0);
        return;
    }
    
    float totalX = 0, totalY = 0;
    for (const Vector2& v : vertices) {
        totalX += v.x;
        totalY += v.y;
    }
    
    center = Vector2(totalX / vertices.size(), totalY / vertices.size());
}

float NavPolygon::GetArea() const {
    if (vertices.size() < 3) return 0.0f;
    
    float area = 0.0f;
    int n = vertices.size();
    
    for (int i = 0; i < n; i++) {
        int j = (i + 1) % n;
        area += vertices[i].x * vertices[j].y;
        area -= vertices[j].x * vertices[i].y;
    }
    
    return std::abs(area) * 0.5f;
}

// NavPath implementation
Vector2 NavPath::GetCurrentWaypoint() const {
    if (waypoints.empty()) return Vector2(0, 0);
    if (currentWaypointIndex >= waypoints.size()) return waypoints.back();
    return waypoints[currentWaypointIndex];
}

bool NavPath::HasNextWaypoint() const {
    return currentWaypointIndex < waypoints.size();
}

void NavPath::AdvanceWaypoint() {
    if (currentWaypointIndex < waypoints.size()) {
        currentWaypointIndex++;
    }
}

bool NavPath::IsComplete() const {
    return currentWaypointIndex >= waypoints.size();
}

// NavMesh implementation
NavMesh::NavMesh(Map* map) : map(map), gridWidth(0), gridHeight(0) {}

void NavMesh::Build() {
    if (!map) return;
    
    polygons.clear();
    
    gridWidth = map->GetWidth();
    gridHeight = map->GetHeight();
    
    spatialGrid.resize(gridWidth);
    for (int i = 0; i < gridWidth; i++) {
        spatialGrid[i].resize(gridHeight, -1);
    }
    
    BuildPolygons();
    MergeAdjacentRectangles();
    ConnectNeighbors();

    // Debug stats
    int isolated = 0;
    for (const auto& poly : polygons) {
        if (poly.neighbors.empty()) {
            isolated++;
        }
    }

    std::cout << "NavMesh build complete:\n";
    std::cout << "  polygons: " << polygons.size() << "\n";
    std::cout << "  isolated polygons: " << isolated << "\n";
}

void NavMesh::BuildPolygons() {
    // Start with individual walkable tiles as 1x1 rectangles
    std::vector<std::vector<bool>> used(gridHeight, std::vector<bool>(gridWidth, false));
    
    int polyId = 0;
    
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            if (used[y][x]) continue;
            if (!map->IsWalkable(x, y)) continue;
            if (map->IsTileBlockedByStaticEntity(x, y, -1)) continue;
            
            // Find maximal rectangle starting from (x, y)
            int maxWidth = 1;
            int maxHeight = 1;
            
            // Expand width
            while (x + maxWidth < gridWidth && 
                   !used[y][x + maxWidth] &&
                   map->IsWalkable(x + maxWidth, y) &&
                   !map->IsTileBlockedByStaticEntity(x + maxWidth, y, -1)) {
                maxWidth++;
            }
            
            // Expand height (while maintaining width)
            bool canExpand = true;
            while (y + maxHeight < gridHeight && canExpand) {
                for (int dx = 0; dx < maxWidth; dx++) {
                    if (used[y + maxHeight][x + dx] ||
                        !map->IsWalkable(x + dx, y + maxHeight) ||
                        map->IsTileBlockedByStaticEntity(x + dx, y + maxHeight, -1)) {
                        canExpand = false;
                        break;
                    }
                }
                if (canExpand) maxHeight++;
            }
            
            // Create polygon for this rectangle
            NavPolygon poly(polyId);
            
            float worldX = x * GRID_CELL_SIZE;
            float worldY = y * GRID_CELL_SIZE;
            float worldW = maxWidth * GRID_CELL_SIZE;
            float worldH = maxHeight * GRID_CELL_SIZE;
            
            poly.vertices = {
                Vector2(worldX, worldY),
                Vector2(worldX + worldW, worldY),
                Vector2(worldX + worldW, worldY + worldH),
                Vector2(worldX, worldY + worldH)
            };
            
            poly.CalculateCenter();
            polygons.push_back(poly);
            
            // Mark tiles as used and update spatial grid
            for (int dy = 0; dy < maxHeight; dy++) {
                for (int dx = 0; dx < maxWidth; dx++) {
                    used[y + dy][x + dx] = true;
                    spatialGrid[x + dx][y + dy] = polyId;
                }
            }
            
            polyId++;
        }
    }
}

void NavMesh::MergeAdjacentRectangles() {
    bool merged = true;
    int iterations = 0;
    const int MAX_ITERATIONS = 10; // Prevent infinite loops
    
    while (merged && iterations < MAX_ITERATIONS) {
        merged = false;
        iterations++;
        
        for (size_t i = 0; i < polygons.size(); i++) {
            if (polygons[i].id == -1) continue; // Already merged
            
            for (size_t j = i + 1; j < polygons.size(); j++) {
                if (polygons[j].id == -1) continue; // Already merged
                
                // Try to merge polygon i and j
                if (CanMergePolygons(i, j)) {
                    MergeTwoPolygons(i, j);
                    merged = true;
                    break; // Restart from beginning after a merge
                }
            }
            
            if (merged) break;
        }
    }
    
    // Remove merged (invalidated) polygons and reindex
    CompactPolygons();
}


bool NavMesh::CanMergePolygons(int idx1, int idx2) const {
    if (idx1 < 0 || idx1 >= polygons.size()) return false;
    if (idx2 < 0 || idx2 >= polygons.size()) return false;
    
    const NavPolygon& p1 = polygons[idx1];
    const NavPolygon& p2 = polygons[idx2];
    
    // Check if they share an edge
    if (!PolygonsShareEdge(idx1, idx2)) return false;
    
    // Check if result would be convex
    std::vector<Vector2> merged = MergeVertices(p1, p2);
    return IsConvex(merged);
}

std::vector<Vector2> NavMesh::MergeVertices(const NavPolygon& p1, const NavPolygon& p2) const {
    // Find all unique vertices from both polygons
    std::vector<Vector2> allVertices;
    
    for (const Vector2& v : p1.vertices) {
        allVertices.push_back(v);
    }
    
    for (const Vector2& v : p2.vertices) {
        // Only add if not already in list
        bool exists = false;
        for (const Vector2& existing : allVertices) {
            if (existing.Distance(v) < 1.0f) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            allVertices.push_back(v);
        }
    }
    
    // Sort vertices by angle from centroid (creates convex hull)
    Vector2 centroid(0, 0);
    for (const Vector2& v : allVertices) {
        centroid.x += v.x;
        centroid.y += v.y;
    }
    centroid.x /= allVertices.size();
    centroid.y /= allVertices.size();
    
    std::sort(allVertices.begin(), allVertices.end(), 
        [centroid](const Vector2& a, const Vector2& b) {
            float angleA = atan2(a.y - centroid.y, a.x - centroid.x);
            float angleB = atan2(b.y - centroid.y, b.x - centroid.x);
            return angleA < angleB;
        });
    
    return allVertices;
}

bool NavMesh::IsConvex(const std::vector<Vector2>& vertices) const {
    if (vertices.size() < 3) return false;
    if (vertices.size() > 8) return false; // Limit complexity
    
    int n = vertices.size();
    bool hasPositive = false;
    bool hasNegative = false;
    
    for (int i = 0; i < n; i++) {
        Vector2 v1 = vertices[i];
        Vector2 v2 = vertices[(i + 1) % n];
        Vector2 v3 = vertices[(i + 2) % n];
        
        Vector2 edge1 = v2 - v1;
        Vector2 edge2 = v3 - v2;
        
        // Cross product
        float cross = edge1.x * edge2.y - edge1.y * edge2.x;
        
        if (cross > 0.01f) hasPositive = true;
        if (cross < -0.01f) hasNegative = true;
    }
    
    // Convex if all cross products have same sign
    return !(hasPositive && hasNegative);
}

void NavMesh::MergeTwoPolygons(int idx1, int idx2) {
    NavPolygon& p1 = polygons[idx1];
    NavPolygon& p2 = polygons[idx2];
    
    // Merge vertices
    std::vector<Vector2> merged = MergeVertices(p1, p2);
    
    p1.vertices = merged;
    p1.CalculateCenter();
    
    // Merge neighbors (same as before)
    std::unordered_set<int> mergedNeighbors;
    
    for (int n : p1.neighbors) {
        if (n != idx2) mergedNeighbors.insert(n);
    }
    
    for (int n : p2.neighbors) {
        if (n != idx1) mergedNeighbors.insert(n);
    }
    
    p1.neighbors.clear();
    for (int n : mergedNeighbors) {
        p1.neighbors.push_back(n);
        
        NavPolygon& neighbor = polygons[n];
        for (int& nb : neighbor.neighbors) {
            if (nb == idx2) {
                nb = idx1;
            }
        }
    }
    
    // Mark p2 as merged
    p2.id = -1;
    p2.vertices.clear();
    p2.neighbors.clear();
    
    // Update spatial grid
    UpdateSpatialGridForPolygon(idx1, p1);
    ClearSpatialGridForPolygon(idx2);
}

void NavMesh::UpdateSpatialGridForPolygon(int polyId, const NavPolygon& poly) {
    Rect box = GetPolygonBoundingBox(poly);
    
    int startX = box.x / GRID_CELL_SIZE;
    int startY = box.y / GRID_CELL_SIZE;
    int endX = (box.x + box.width) / GRID_CELL_SIZE;
    int endY = (box.y + box.height) / GRID_CELL_SIZE;
    
    for (int y = startY; y <= endY && y < gridHeight; y++) {
        for (int x = startX; x <= endX && x < gridWidth; x++) {
            if (x >= 0 && x < gridWidth && y >= 0 && y < gridHeight) {
                spatialGrid[x][y] = polyId;
            }
        }
    }
}

Rect NavMesh::GetPolygonBoundingBox(const NavPolygon& poly) const {
    if (poly.vertices.empty()) return Rect(0, 0, 0, 0);

    float minX = poly.vertices[0].x;
    float minY = poly.vertices[0].y;
    float maxX = poly.vertices[0].x;
    float maxY = poly.vertices[0].y;

    for (const Vector2& v : poly.vertices) {
        minX = std::min(minX, v.x);
        minY = std::min(minY, v.y);
        maxX = std::max(maxX, v.x);
        maxY = std::max(maxY, v.y);
    }

    return Rect((int)minX, (int)minY, (int)(maxX - minX), (int)(maxY - minY));
}

void NavMesh::ClearSpatialGridForPolygon(int polyId) {
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            if (spatialGrid[x][y] == polyId) {
                spatialGrid[x][y] = -1;
            }
        }
    }
}


void NavMesh::ConnectNeighbors() {
    // Clear old neighbors first
    for (auto& poly : polygons) {
        poly.neighbors.clear();
    }

    for (size_t i = 0; i < polygons.size(); i++) {
        for (size_t j = i + 1; j < polygons.size(); j++) {
            if (PolygonsShareEdge((int)i, (int)j)) {
                polygons[i].neighbors.push_back((int)j);
                polygons[j].neighbors.push_back((int)i);
            }
        }
    }
}

void NavMesh::CompactPolygons() {
    std::vector<NavPolygon> compacted;
    std::unordered_map<int, int> oldToNew; // Map old ID to new ID
    
    // Collect valid polygons and build ID mapping
    for (size_t i = 0; i < polygons.size(); i++) {
        if (polygons[i].id != -1) {
            int newId = compacted.size();
            oldToNew[i] = newId;
            
            NavPolygon poly = polygons[i];
            poly.id = newId;
            compacted.push_back(poly);
        }
    }
    
    // Update neighbor references
    for (NavPolygon& poly : compacted) {
        for (int& neighbor : poly.neighbors) {
            if (oldToNew.count(neighbor)) {
                neighbor = oldToNew[neighbor];
            }
        }
    }
    
    // Update spatial grid
    for (int y = 0; y < gridHeight; y++) {
        for (int x = 0; x < gridWidth; x++) {
            int oldId = spatialGrid[x][y];
            if (oldId >= 0 && oldToNew.count(oldId)) {
                spatialGrid[x][y] = oldToNew[oldId];
            }
        }
    }
    
    polygons = std::move(compacted);
}

bool NavMesh::PolygonsShareEdge(int poly1, int poly2) const {
    if (poly1 < 0 || poly1 >= (int)polygons.size()) return false;
    if (poly2 < 0 || poly2 >= (int)polygons.size()) return false;

    const Rect a = GetPolygonBoundingBox(polygons[poly1]);
    const Rect b = GetPolygonBoundingBox(polygons[poly2]);

    // Vertical adjacency:
    // right side of A touches left side of B, or vice versa
    bool verticalTouch =
        (a.x + a.width == b.x || b.x + b.width == a.x);

    if (verticalTouch) {
        int overlapTop = std::max(a.y, b.y);
        int overlapBottom = std::min(a.y + a.height, b.y + b.height);

        // Need actual positive overlap, not just corner touching
        if (overlapBottom > overlapTop) {
            return true;
        }
    }

    // Horizontal adjacency:
    // bottom side of A touches top side of B, or vice versa
    bool horizontalTouch =
        (a.y + a.height == b.y || b.y + b.height == a.y);

    if (horizontalTouch) {
        int overlapLeft = std::max(a.x, b.x);
        int overlapRight = std::min(a.x + a.width, b.x + b.width);

        // Need actual positive overlap, not just corner touching
        if (overlapRight > overlapLeft) {
            return true;
        }
    }

    return false;
}

NavMesh::Edge NavMesh::GetSharedEdge(int poly1, int poly2) const {
    if (poly1 < 0 || poly1 >= (int)polygons.size()) return Edge(Vector2(0,0), Vector2(0,0));
    if (poly2 < 0 || poly2 >= (int)polygons.size()) return Edge(Vector2(0,0), Vector2(0,0));
    
    const Rect a = GetPolygonBoundingBox(polygons[poly1]);
    const Rect b = GetPolygonBoundingBox(polygons[poly2]);
    
    // Vertical shared boundary (A is to the left of B, or vice versa)
    if (a.x + a.width == b.x) {
        // A's right edge == B's left edge
        float sharedX = (float)(a.x + a.width);
        float top = (float)std::max(a.y, b.y);
        float bottom = (float)std::min(a.y + a.height, b.y + b.height);
        
        if (bottom > top) {
            // Portal goes top to bottom
            return Edge(Vector2(sharedX, top), Vector2(sharedX, bottom));
        }
    }
    
    if (b.x + b.width == a.x) {
        // B's right edge == A's left edge
        float sharedX = (float)(b.x + b.width);
        float top = (float)std::max(a.y, b.y);
        float bottom = (float)std::min(a.y + a.height, b.y + b.height);
        
        if (bottom > top) {
            return Edge(Vector2(sharedX, top), Vector2(sharedX, bottom));
        }
    }
    
    // Horizontal shared boundary (A is above B, or vice versa)
    if (a.y + a.height == b.y) {
        // A's bottom edge == B's top edge
        float sharedY = (float)(a.y + a.height);
        float left = (float)std::max(a.x, b.x);
        float right = (float)std::min(a.x + a.width, b.x + b.width);
        
        if (right > left) {
            // Portal goes left to right
            return Edge(Vector2(left, sharedY), Vector2(right, sharedY));
        }
    }
    
    if (b.y + b.height == a.y) {
        // B's bottom edge == A's top edge
        float sharedY = (float)(b.y + b.height);
        float left = (float)std::max(a.x, b.x);
        float right = (float)std::min(a.x + a.width, b.x + b.width);
        
        if (right > left) {
            return Edge(Vector2(left, sharedY), Vector2(right, sharedY));
        }
    }
    
    // No shared edge found
    return Edge(Vector2(0, 0), Vector2(0, 0));
}

int NavMesh::GetPolygonAt(const Vector2& position) const {
    int tx = (int)(position.x / GRID_CELL_SIZE);
    int ty = (int)(position.y / GRID_CELL_SIZE);
    
    if (tx >= 0 && tx < gridWidth && ty >= 0 && ty < gridHeight) {
        return spatialGrid[tx][ty];
    }
    
    return -1;
}

const NavPolygon* NavMesh::GetPolygon(int id) const {
    if (id < 0 || id >= polygons.size()) return nullptr;
    return &polygons[id];
}

bool NavMesh::IsPointWalkable(const Vector2& point) const {
    return GetPolygonAt(point) >= 0;
}

NavPath NavMesh::FindPath(const Vector2& start, const Vector2& end) const {
    NavPath result;
    
    int startPoly = GetPolygonAt(start);
    int endPoly = GetPolygonAt(end);
    
    if (startPoly < 0 || endPoly < 0) {
        return result;
    }
    
    if (startPoly == endPoly) {
        result.polygonIds = {startPoly};
        result.waypoints = {end};
        result.currentWaypointIndex = 0;
        return result;
    }
    
    // A* on polygons
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
    
    gCost[startPoly] = 0;
    fCost[startPoly] = Heuristic(startPoly, endPoly);
    open.push(startPoly);
    
    while (!open.empty()) {
        int current = open.top();
        open.pop();
        
        if (current == endPoly) {
            // Reconstruct polygon path
            std::vector<int> polyPath;
            int node = endPoly;
            while (node != startPoly) {
                polyPath.push_back(node);
                node = parent[node];
            }
            polyPath.push_back(startPoly);
            std::reverse(polyPath.begin(), polyPath.end());
            
            result.polygonIds = polyPath;
            result.waypoints = FunnelAlgorithm(polyPath, start, end);
            result.currentWaypointIndex = 0;
            return result;
        }
        
        if (closed.find(current) != closed.end()) continue;
        closed.insert(current);
        
        for (int neighbor : polygons[current].neighbors) {
            if (closed.find(neighbor) != closed.end()) continue;
            
            float edgeCost = polygons[current].center.Distance(polygons[neighbor].center);
            float newGCost = gCost[current] + edgeCost;
            
            if (!gCost.count(neighbor) || newGCost < gCost[neighbor]) {
                gCost[neighbor] = newGCost;
                fCost[neighbor] = newGCost + Heuristic(neighbor, endPoly);
                parent[neighbor] = current;
                open.push(neighbor);
            }
        }
    }
    
    return result;
}

float NavMesh::Heuristic(int polyA, int polyB) const {
    if (polyA < 0 || polyA >= polygons.size() || polyB < 0 || polyB >= polygons.size()) {
        return std::numeric_limits<float>::max();
    }
    
    return polygons[polyA].center.Distance(polygons[polyB].center);
}

std::vector<Vector2> NavMesh::FunnelAlgorithm(const std::vector<int>& polygonPath,
                                               const Vector2& start,
                                               const Vector2& end) const {
    std::vector<Vector2> waypoints;
    
    if (polygonPath.empty()) {
        waypoints.push_back(start);
        waypoints.push_back(end);
        return waypoints;
    }
    
    if (polygonPath.size() == 1) {
        // Same polygon - go directly
        waypoints.push_back(start);
        waypoints.push_back(end);
        return waypoints;
    }
    
    // Step 1: Collect all portal edges between consecutive polygons
    struct Portal {
        Vector2 left;
        Vector2 right;
    };
    
    std::vector<Portal> portals;
    
    for (size_t i = 0; i < polygonPath.size() - 1; i++) {
        Edge edge = GetSharedEdge(polygonPath[i], polygonPath[i + 1]);
        portals.push_back({edge.a, edge.b});
    }
    
    if (portals.empty()) {
        waypoints.push_back(start);
        waypoints.push_back(end);
        return waypoints;
    }
    
    // Step 2: Build path by finding closest point on each portal
    // from the current position, creating the shortest path through portals
    
    waypoints.push_back(start);
    
    Vector2 currentPos = start;
    
    for (size_t i = 0; i < portals.size(); i++) {
        Vector2 portalA = portals[i].left;
        Vector2 portalB = portals[i].right;
        
        // Where do we want to go AFTER this portal?
        Vector2 lookAhead;
        if (i + 1 < portals.size()) {
            // Look at the midpoint of the next portal
            lookAhead = Vector2(
                (portals[i + 1].left.x + portals[i + 1].right.x) * 0.5f,
                (portals[i + 1].left.y + portals[i + 1].right.y) * 0.5f
            );
        } else {
            // Last portal - look at the end point
            lookAhead = end;
        }
        
        // Find the best point on this portal to cross
        // Project the ideal line (currentPos -> lookAhead) onto the portal segment
        Vector2 crossPoint = ClosestPointOnSegment(currentPos, lookAhead, 
                                                    portalA, portalB);
        
        // Only add waypoint if it changes direction significantly
        if (waypoints.size() >= 2) {
            Vector2 prevDir = (waypoints.back() - waypoints[waypoints.size() - 2]).Normalized();
            Vector2 newDir = (crossPoint - waypoints.back()).Normalized();
            float dot = prevDir.x * newDir.x + prevDir.y * newDir.y;
            
            // If direction change is small, skip this waypoint
            if (dot > 0.95f) {
                // Nearly straight - update last waypoint instead of adding new one
                waypoints.back() = crossPoint;
                currentPos = crossPoint;
                continue;
            }
        }
        
        // Don't add if too close to previous waypoint
        if (waypoints.back().Distance(crossPoint) > 5.0f) {
            waypoints.push_back(crossPoint);
        }
        
        currentPos = crossPoint;
    }
    
    // Add end point
    if (waypoints.back().Distance(end) > 5.0f) {
        waypoints.push_back(end);
    }
    
    // Step 3: Line-of-sight optimization
    // Remove unnecessary waypoints where we can go directly
    waypoints = OptimizePath(waypoints);
    
    return waypoints;
}

Vector2 NavMesh::ClosestPointOnSegment(const Vector2& from, const Vector2& to,
                                        const Vector2& segA, const Vector2& segB) const {
    // Find the point on segment (segA, segB) that is closest to
    // the line from 'from' to 'to'
    
    Vector2 segDir = segB - segA;
    float segLen = segDir.Length();
    
    if (segLen < 0.01f) {
        return segA; // Degenerate segment
    }
    
    // Direction from 'from' to 'to'
    Vector2 lineDir = to - from;
    
    // Project the ideal crossing point onto the portal segment
    // The ideal crossing is where the line (from -> to) intersects the portal
    
    // Use line-segment intersection
    float denom = lineDir.x * segDir.y - lineDir.y * segDir.x;
    
    if (std::abs(denom) > 0.001f) {
        // Lines are not parallel
        Vector2 diff = segA - from;
        float t = (diff.x * segDir.y - diff.y * segDir.x) / denom;
        float u = (diff.x * lineDir.y - diff.y * lineDir.x) / denom;
        
        // Clamp u to [0, 1] to stay on the portal segment
        u = std::max(0.0f, std::min(1.0f, u));
        
        // Add a small margin from the edges of the portal
        float margin = std::min(8.0f, segLen * 0.1f);
        float marginT = margin / segLen;
        u = std::max(marginT, std::min(1.0f - marginT, u));
        
        return segA + segDir * u;
    }
    
    // Lines are parallel - return closest point on segment to 'from'
    Vector2 toFrom = from - segA;
    float t = (toFrom.x * segDir.x + toFrom.y * segDir.y) / (segLen * segLen);
    t = std::max(0.0f, std::min(1.0f, t));
    
    return segA + segDir * t;
}

std::vector<Vector2> NavMesh::OptimizePath(const std::vector<Vector2>& path) const {
    if (path.size() <= 2) return path;
    
    std::vector<Vector2> optimized;
    optimized.push_back(path[0]);
    
    size_t current = 0;
    
    while (current < path.size() - 1) {
        // Find the farthest point we can reach directly
        size_t farthest = current + 1;
        
        for (size_t test = path.size() - 1; test > current + 1; test--) {
            if (HasLineOfSight(path[current], path[test])) {
                farthest = test;
                break;
            }
        }
        
        optimized.push_back(path[farthest]);
        current = farthest;
    }
    
    return optimized;
}

bool NavMesh::HasLineOfSight(const Vector2& from, const Vector2& to) const {
    // Check if we can walk in a straight line from 'from' to 'to'
    // by sampling points along the line and checking if they're on the navmesh
    
    Vector2 dir = to - from;
    float distance = dir.Length();
    
    if (distance < 1.0f) return true;
    
    float step = 8.0f; // Check every 8 pixels
    int numSteps = (int)(distance / step) + 1;
    
    for (int i = 1; i <= numSteps; i++) {
        float t = (float)i / (float)numSteps;
        Vector2 point = from + dir * t;
        
        if (GetPolygonAt(point) < 0) {
            return false; // Point is not on walkable navmesh
        }
    }
    
    return true;
}

float NavMesh::TriangleArea2(const Vector2& a, const Vector2& b, const Vector2& c) {
    return (b.x - a.x) * (c.y - a.y) - (c.x - a.x) * (b.y - a.y);
}

void NavMesh::DebugRender(Renderer* renderer) const {
    if (!renderer) return;
    
    for (const NavPolygon& poly : polygons) {
        // Draw polygon edges
        for (size_t i = 0; i < poly.vertices.size(); i++) {
            Vector2 a = poly.vertices[i];
            Vector2 b = poly.vertices[(i + 1) % poly.vertices.size()];
            renderer->DrawLine(a, b, glm::vec3(0.0f, 1.0f, 0.0f), 2.0f);
        }
        
        // Draw center
        renderer->DrawCircle(poly.center, 4.0f, glm::vec3(1.0f, 1.0f, 0.0f));
    }
}