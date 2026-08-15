#include "placement.h"
#include <cmath>
#include <algorithm>
#include <queue>
#include <set>
#include <numeric>
#include <unordered_set>
#include <unordered_map>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PlacementStrategy::PlacementStrategy(std::mt19937& rng, PerlinNoise& noise)
    : rng_(rng), noise_(noise) {}

int PlacementStrategy::baseRadius(int mapSize) const {
    int r = (int)(mapSize * 0.08f);
    return std::max(r, 30);
}

PlacementResult PlacementStrategy::place(MapData& map, int numPlayers, PlacementMode mode) {
    switch (mode) {
        case PlacementMode::Circle:    return placeCircle(map, numPlayers);
        case PlacementMode::Organic:   return placeOrganic(map, numPlayers);
        case PlacementMode::Voronoi:   return placeVoronoi(map, numPlayers);
        case PlacementMode::Spiral:    return placeSpiral(map, numPlayers);
        case PlacementMode::Clustered: return placeClustered(map, numPlayers);
        default:                       return placeCircle(map, numPlayers);
    }
}

// ===========================================================================
// Circle: symmetric placement on a circle (original behavior)
// ===========================================================================
PlacementResult PlacementStrategy::placeCircle(MapData& map, int numPlayers) {
    PlacementResult result;
    int w = map.getWidth();
    int h = map.getHeight();
    float centerX = w / 2.0f;
    float centerY = h / 2.0f;
    float radius = std::min(w, h) * 0.35f;
    int areaRadius = baseRadius(std::min(w, h));

    for (int i = 0; i < numPlayers; i++) {
        float angle = 2.0f * (float)M_PI * i / numPlayers - (float)M_PI / 2.0f;
        int cx = (int)(centerX + radius * std::cos(angle));
        int cy = (int)(centerY + radius * std::sin(angle));
        cx = std::clamp(cx, areaRadius + 10, w - areaRadius - 10);
        cy = std::clamp(cy, areaRadius + 10, h - areaRadius - 10);

        StartingArea sa;
        sa.centerX = cx;
        sa.centerY = cy;
        sa.radius = areaRadius;
        sa.playerIndex = i;
        result.areas.push_back(sa);
    }
    return result;
}

// ===========================================================================
// Organic: noise-distorted natural shapes, slightly randomized positions
// ===========================================================================
PlacementResult PlacementStrategy::placeOrganic(MapData& map, int numPlayers) {
    PlacementResult result;
    int w = map.getWidth();
    int h = map.getHeight();
    float centerX = w / 2.0f;
    float centerY = h / 2.0f;
    float radius = std::min(w, h) * 0.35f;
    int areaRadius = baseRadius(std::min(w, h));

    std::uniform_real_distribution<float> jitter(-0.12f, 0.12f);
    std::uniform_real_distribution<float> radiusJitter(0.85f, 1.15f);
    std::uniform_real_distribution<float> noiseOffset(0.0f, 1000.0f);

    for (int i = 0; i < numPlayers; i++) {
        float baseAngle = 2.0f * (float)M_PI * i / numPlayers - (float)M_PI / 2.0f;
        float angle = baseAngle + jitter(rng_) * (2.0f * (float)M_PI / numPlayers);
        float r = radius * radiusJitter(rng_);

        int cx = (int)(centerX + r * std::cos(angle));
        int cy = (int)(centerY + r * std::sin(angle));
        cx = std::clamp(cx, areaRadius + 15, w - areaRadius - 15);
        cy = std::clamp(cy, areaRadius + 15, h - areaRadius - 15);

        StartingArea sa;
        sa.centerX = cx;
        sa.centerY = cy;
        sa.radius = areaRadius;
        sa.playerIndex = i;

        // Generate organic shape using noise
        float noiseOff = noiseOffset(rng_);
        generateOrganicShape(sa, map, 0.08f + noiseOff * 0.00001f, 0.45f);

        result.areas.push_back(sa);
    }

    return result;
}

void PlacementStrategy::generateOrganicShape(StartingArea& area, MapData& map,
                                               float noiseScale, float distortion) {
    area.shapeTiles.clear();
    area.boundaryTiles.clear();

    int r = area.radius;
    float rFloat = (float)r;

    // Use noise to distort the boundary of the circle
    // For each angle, compute a radius offset using Perlin noise
    // Then fill all tiles inside the distorted boundary

    // Pre-compute boundary radii at fine angular resolution
    int numAngles = 360;
    std::vector<float> boundaryRadius(numAngles);

    // Use a unique noise offset per player so shapes differ
    float playerOffset = area.playerIndex * 137.5f;

    for (int a = 0; a < numAngles; a++) {
        float theta = 2.0f * (float)M_PI * a / numAngles;

        // Multi-octave noise for natural look
        float n1 = (float)noise_.octaveNoise(
            std::cos(theta) * 3.0f + playerOffset,
            std::sin(theta) * 3.0f + playerOffset, 4, 0.5);
        float n2 = (float)noise_.octaveNoise(
            std::cos(theta * 2.0f) * 5.0f + playerOffset + 50.0f,
            std::sin(theta * 2.0f) * 5.0f + playerOffset + 50.0f, 3, 0.6);

        // Combine: base radius with noise distortion
        float noise_val = n1 * 0.7f + n2 * 0.3f;
        float br = rFloat * (1.0f + distortion * noise_val);
        br = std::clamp(br, rFloat * 0.5f, rFloat * 1.4f);
        boundaryRadius[a] = br;
    }

    // Smooth the boundary to avoid jagged edges
    std::vector<float> smoothed(numAngles);
    int smoothWindow = 8;
    for (int a = 0; a < numAngles; a++) {
        float sum = 0;
        for (int k = -smoothWindow; k <= smoothWindow; k++) {
            int idx = (a + k + numAngles) % numAngles;
            sum += boundaryRadius[idx];
        }
        smoothed[a] = sum / (2 * smoothWindow + 1);
    }
    boundaryRadius = smoothed;

    // Fill shape tiles
    std::set<std::pair<int,int>> shapeSet;

    for (int dy = -(int)(rFloat * 1.5f); dy <= (int)(rFloat * 1.5f); dy++) {
        for (int dx = -(int)(rFloat * 1.5f); dx <= (int)(rFloat * 1.5f); dx++) {
            float dist = std::sqrt((float)(dx * dx + dy * dy));
            if (dist < 1.0f) {
                int px = area.centerX + dx;
                int py = area.centerY + dy;
                if (map.inBounds(px, py)) {
                    shapeSet.insert({px, py});
                }
                continue;
            }

            float theta = std::atan2((float)dy, (float)dx);
            if (theta < 0) theta += 2.0f * (float)M_PI;

            float angFrac = theta / (2.0f * (float)M_PI) * numAngles;
            int a0 = (int)angFrac % numAngles;
            int a1 = (a0 + 1) % numAngles;
            float t = angFrac - std::floor(angFrac);
            float br = boundaryRadius[a0] * (1.0f - t) + boundaryRadius[a1] * t;

            if (dist <= br) {
                int px = area.centerX + dx;
                int py = area.centerY + dy;
                if (map.inBounds(px, py)) {
                    shapeSet.insert({px, py});
                }
            }
        }
    }

    area.shapeTiles.assign(shapeSet.begin(), shapeSet.end());

    // Compute boundary: tiles in shape that have a neighbor NOT in shape
    int thickness = std::max(3, area.radius / 8);
    computeBoundary(area, map, thickness);
}

void PlacementStrategy::computeBoundary(StartingArea& area, MapData& map, int thickness) {
    area.boundaryTiles.clear();

    if (!area.hasShape()) return;

    std::set<std::pair<int,int>> shapeSet(area.shapeTiles.begin(), area.shapeTiles.end());

    // Find outer boundary
    std::set<std::pair<int,int>> outerEdge;
    for (auto& [x, y] : area.shapeTiles) {
        int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
        int dy[] = {0, 0, 1, -1, 1, -1, 1, -1};
        for (int d = 0; d < 8; d++) {
            int nx = x + dx[d];
            int ny = y + dy[d];
            if (shapeSet.find({nx, ny}) == shapeSet.end()) {
                outerEdge.insert({x, y});
                break;
            }
        }
    }

    // Expand boundary inward by thickness
    // BFS from outer edge inward
    std::set<std::pair<int,int>> boundarySet;
    std::queue<std::pair<std::pair<int,int>, int>> q;
    std::set<std::pair<int,int>> visited;

    for (auto& p : outerEdge) {
        q.push({p, 0});
        visited.insert(p);
        boundarySet.insert(p);
    }

    while (!q.empty()) {
        auto [pos, depth] = q.front(); q.pop();
        if (depth >= thickness) continue;

        int dx[] = {1, -1, 0, 0};
        int dy[] = {0, 0, 1, -1};
        for (int d = 0; d < 4; d++) {
            int nx = pos.first + dx[d];
            int ny = pos.second + dy[d];
            std::pair<int,int> np = {nx, ny};
            if (visited.find(np) == visited.end() && shapeSet.find(np) != shapeSet.end()) {
                visited.insert(np);
                boundarySet.insert(np);
                q.push({np, depth + 1});
            }
        }
    }

    area.boundaryTiles.assign(boundarySet.begin(), boundarySet.end());
}

// ===========================================================================
// Voronoi: positions on jittered circle, areas defined by Voronoi cells
// ===========================================================================
PlacementResult PlacementStrategy::placeVoronoi(MapData& map, int numPlayers) {
    PlacementResult result;
    int w = map.getWidth();
    int h = map.getHeight();
    float centerX = w / 2.0f;
    float centerY = h / 2.0f;
    float radius = std::min(w, h) * 0.33f;
    int areaRadius = baseRadius(std::min(w, h));

    std::uniform_real_distribution<float> jitter(-0.08f, 0.08f);

    // Place player centers
    for (int i = 0; i < numPlayers; i++) {
        float baseAngle = 2.0f * (float)M_PI * i / numPlayers - (float)M_PI / 2.0f;
        float angle = baseAngle + jitter(rng_) * (2.0f * (float)M_PI / numPlayers);
        float r = radius * (1.0f + jitter(rng_) * 0.5f);

        int cx = (int)(centerX + r * std::cos(angle));
        int cy = (int)(centerY + r * std::sin(angle));
        cx = std::clamp(cx, areaRadius + 10, w - areaRadius - 10);
        cy = std::clamp(cy, areaRadius + 10, h - areaRadius - 10);

        StartingArea sa;
        sa.centerX = cx;
        sa.centerY = cy;
        sa.radius = areaRadius;
        sa.playerIndex = i;
        result.areas.push_back(sa);
    }

    // Compute Voronoi cells
    int maxCellRadius = (int)(areaRadius * 1.5f);
    computeVoronoiCells(result.areas, map, maxCellRadius);

    return result;
}

void PlacementStrategy::computeVoronoiCells(std::vector<StartingArea>& areas, MapData& map,
                                              int maxCellRadius) {
    int w = map.getWidth();
    int h = map.getHeight();
    int n = (int)areas.size();

    // For efficiency, only compute within a bounding box around all centers
    // expanded by maxCellRadius
    int minX = w, minY = h, maxX = 0, maxY = 0;
    for (auto& sa : areas) {
        minX = std::min(minX, sa.centerX - maxCellRadius);
        minY = std::min(minY, sa.centerY - maxCellRadius);
        maxX = std::max(maxX, sa.centerX + maxCellRadius);
        maxY = std::max(maxY, sa.centerY + maxCellRadius);
    }
    minX = std::max(0, minX);
    minY = std::max(0, minY);
    maxX = std::min(w - 1, maxX);
    maxY = std::min(h - 1, maxY);

    // Add noise to distances for more natural cell shapes
    std::uniform_real_distribution<float> noiseDist(0.0f, 500.0f);
    std::vector<float> noiseOffsets(n);
    for (int i = 0; i < n; i++) {
        noiseOffsets[i] = noiseDist(rng_);
    }

    // For each cell, collect tiles
    std::vector<std::set<std::pair<int,int>>> cellSets(n);

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            // Find closest center with noise-distorted distance
            float minDist = 1e18f;
            int closest = -1;

            for (int i = 0; i < n; i++) {
                float dx = (float)(x - areas[i].centerX);
                float dy = (float)(y - areas[i].centerY);
                float baseDist = std::sqrt(dx * dx + dy * dy);

                if (baseDist > maxCellRadius * 1.2f) continue;

                // Add Perlin noise distortion to distance for organic cell edges
                float noiseVal = (float)noise_.octaveNoise(
                    x * 0.02f + noiseOffsets[i],
                    y * 0.02f + noiseOffsets[i], 3, 0.5) * maxCellRadius * 0.25f;
                float dist = baseDist + noiseVal;

                if (dist < minDist) {
                    minDist = dist;
                    closest = i;
                }
            }

            if (closest >= 0) {
                // Also check actual distance to closest center is within maxCellRadius
                float dx = (float)(x - areas[closest].centerX);
                float dy = (float)(y - areas[closest].centerY);
                float actualDist = std::sqrt(dx * dx + dy * dy);
                if (actualDist <= maxCellRadius) {
                    cellSets[closest].insert({x, y});
                }
            }
        }
    }

    // Store results
    for (int i = 0; i < n; i++) {
        areas[i].shapeTiles.assign(cellSets[i].begin(), cellSets[i].end());
        int thickness = std::max(3, areas[i].radius / 7);
        computeBoundary(areas[i], map, thickness);
    }
}

// ===========================================================================
// Spiral: golden-angle spiral placement (Fibonacci-like)
// ===========================================================================
PlacementResult PlacementStrategy::placeSpiral(MapData& map, int numPlayers) {
    PlacementResult result;
    int w = map.getWidth();
    int h = map.getHeight();
    float centerX = w / 2.0f;
    float centerY = h / 2.0f;
    int areaRadius = baseRadius(std::min(w, h));

    float goldenAngle = (float)M_PI * (3.0f - std::sqrt(5.0f)); // ~137.5 degrees

    float maxRadius = std::min(w, h) * 0.40f;

    std::uniform_real_distribution<float> jitter(-0.05f, 0.05f);

    for (int i = 0; i < numPlayers; i++) {
        // Fibonacci spiral: radius grows with sqrt of index, angle by golden ratio
        float t = (float)(i + 1) / (float)(numPlayers + 1);
        float r = maxRadius * std::sqrt(t) * (1.0f + jitter(rng_));
        float angle = goldenAngle * (i + 1) + jitter(rng_) * 0.3f;

        int cx = (int)(centerX + r * std::cos(angle));
        int cy = (int)(centerY + r * std::sin(angle));
        cx = std::clamp(cx, areaRadius + 10, w - areaRadius - 10);
        cy = std::clamp(cy, areaRadius + 10, h - areaRadius - 10);

        StartingArea sa;
        sa.centerX = cx;
        sa.centerY = cy;
        sa.radius = areaRadius;
        sa.playerIndex = i;

        // Use organic shape for spiral too
        float playerOffset = i * 200.0f + 42.0f;
        generateOrganicShape(sa, map, 0.07f + playerOffset * 0.000005f, 0.35f);

        result.areas.push_back(sa);
    }

    return result;
}

// ===========================================================================
// Clustered: players in 2 or more clusters (team-like)
// ===========================================================================
PlacementResult PlacementStrategy::placeClustered(MapData& map, int numPlayers) {
    PlacementResult result;
    int w = map.getWidth();
    int h = map.getHeight();
    float centerX = w / 2.0f;
    float centerY = h / 2.0f;
    int areaRadius = baseRadius(std::min(w, h));

    // Determine number of clusters (2-4 depending on player count)
    int numClusters = 2;
    if (numPlayers >= 6) numClusters = 3;
    if (numPlayers >= 8) numClusters = 4;

    // Place cluster centers on a circle
    float clusterRadius = std::min(w, h) * 0.30f;
    std::vector<std::pair<float, float>> clusterCenters;

    std::uniform_real_distribution<float> jitter(-0.1f, 0.1f);

    for (int c = 0; c < numClusters; c++) {
        float angle = 2.0f * (float)M_PI * c / numClusters - (float)M_PI / 2.0f;
        float ccx = centerX + clusterRadius * std::cos(angle);
        float ccy = centerY + clusterRadius * std::sin(angle);
        clusterCenters.push_back({ccx, ccy});
    }

    // Distribute players among clusters
    // Spread as evenly as possible
    std::vector<int> clusterAssignment(numPlayers);
    for (int i = 0; i < numPlayers; i++) {
        clusterAssignment[i] = i % numClusters;
    }

    // Count per cluster
    std::vector<int> perCluster(numClusters, 0);
    for (int i = 0; i < numPlayers; i++) {
        perCluster[clusterAssignment[i]]++;
    }

    // Place players in subcircles around their cluster center
    std::vector<int> clusterIdx(numClusters, 0);
    float subRadius = std::min(w, h) * 0.08f;

    for (int i = 0; i < numPlayers; i++) {
        int cluster = clusterAssignment[i];
        int indexInCluster = clusterIdx[cluster]++;
        int totalInCluster = perCluster[cluster];

        float angle;
        float r;
        if (totalInCluster == 1) {
            angle = 0;
            r = 0;
        } else {
            angle = 2.0f * (float)M_PI * indexInCluster / totalInCluster;
            r = subRadius;
        }

        float ccx = clusterCenters[cluster].first;
        float ccy = clusterCenters[cluster].second;

        int cx = (int)(ccx + r * std::cos(angle) + jitter(rng_) * areaRadius);
        int cy = (int)(ccy + r * std::sin(angle) + jitter(rng_) * areaRadius);
        cx = std::clamp(cx, areaRadius + 15, w - areaRadius - 15);
        cy = std::clamp(cy, areaRadius + 15, h - areaRadius - 15);

        StartingArea sa;
        sa.centerX = cx;
        sa.centerY = cy;
        sa.radius = areaRadius;
        sa.playerIndex = i;

        // Organic shape
        float playerOffset = i * 171.0f + cluster * 500.0f;
        generateOrganicShape(sa, map, 0.06f + playerOffset * 0.000003f, 0.4f);

        result.areas.push_back(sa);
    }

    // Ensure no overlapping starting positions
    //ensureMinDistance(
    //    *reinterpret_cast<std::vector<std::pair<int,int>>*>(nullptr), // unused in this overload
    //    areaRadius * 2, w, h, areaRadius + 10);

    return result;
}

void PlacementStrategy::ensureMinDistance(std::vector<std::pair<int,int>>& positions,
                                           int minDist, int mapW, int mapH, int margin) {
    // This is used only if positions need adjustment
    // For Clustered mode, we rely on the subcircle placement being adequate
    // If needed, could add repulsion logic here
}