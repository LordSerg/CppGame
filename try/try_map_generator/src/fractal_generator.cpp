#include "fractal_generator.h"
#include "pathfinding.h"
#include <cmath>
#include <queue>
#include <set>
#include <algorithm>
#include <numeric>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

FractalGenerator::FractalGenerator(std::mt19937& rng) : rng_(rng) {}

void FractalGenerator::carveRiverPoint(MapData& map, int cx, int cy, float width,
                                        std::vector<std::pair<int,int>>& waterTiles) {
    int r = (int)std::ceil(width);
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            if (dx * dx + dy * dy <= r * r) {
                int nx = cx + dx;
                int ny = cy + dy;
                if (map.inBounds(nx, ny) && map.getTile(nx, ny) != TileType::Water
                    && map.getTile(nx, ny) != TileType::StartingPoint) {
                    map.setTile(nx, ny, TileType::Water);
                    waterTiles.push_back({nx, ny});
                }
            }
        }
    }
}

void FractalGenerator::growRiverBranch(MapData& map, float x, float y, float angle,
                                        float width, int depth, int maxDepth,
                                        std::vector<std::pair<int,int>>& waterTiles) {
    if (depth >= maxDepth || width < 1.0f) return;

    std::uniform_real_distribution<float> angleDist(-0.3f, 0.3f);
    std::uniform_real_distribution<float> branchProb(0.0f, 1.0f);
    std::uniform_real_distribution<float> branchAngle(0.4f, 1.2f);

    float stepSize = 2.0f;
    int steps = (int)(std::max(20.0f, (float)map.getWidth() * 0.15f / (depth + 1)));

    for (int i = 0; i < steps; i++) {
        int ix = (int)x;
        int iy = (int)y;

        if (!map.inBounds(ix, iy)) break;

        // Don't carve through starting areas
        if (!isNearStartArea(map, ix, iy, (int)(width + 5))) {
            carveRiverPoint(map, ix, iy, width, waterTiles);
        }

        angle += angleDist(rng_);
        x += std::cos(angle) * stepSize;
        y += std::sin(angle) * stepSize;

        // Branch probability
        if (branchProb(rng_) < 0.02f && depth < maxDepth - 1) {
            float bAngle = angle + (branchProb(rng_) > 0.5f ? 1.0f : -1.0f) * branchAngle(rng_);
            growRiverBranch(map, x, y, bAngle, width * 0.6f, depth + 1, maxDepth, waterTiles);
        }

        // Gradually reduce width
        width *= 0.998f;
    }
}

void FractalGenerator::generateRiverSystem(MapData& map, int numSources) {
    int w = map.getWidth();
    int h = map.getHeight();
    std::vector<std::pair<int,int>> waterTiles;

    std::uniform_int_distribution<int> edgeDist(0, 3);
    std::uniform_real_distribution<float> posDist(0.1f, 0.9f);

    float baseWidth = std::max(2.0f, w / 150.0f);
    int maxDepth = 4;

    for (int s = 0; s < numSources; s++) {
        float sx, sy, angle;
        int edge = edgeDist(rng_);

        switch (edge) {
            case 0: // top
                sx = posDist(rng_) * w;
                sy = 0;
                angle = M_PI / 2.0f;
                break;
            case 1: // bottom
                sx = posDist(rng_) * w;
                sy = h - 1;
                angle = -M_PI / 2.0f;
                break;
            case 2: // left
                sx = 0;
                sy = posDist(rng_) * h;
                angle = 0;
                break;
            default: // right
                sx = w - 1;
                sy = posDist(rng_) * h;
                angle = M_PI;
                break;
        }

        std::uniform_real_distribution<float> ad(-0.5f, 0.5f);
        angle += ad(rng_);

        growRiverBranch(map, sx, sy, angle, baseWidth, 0, maxDepth, waterTiles);
    }

    // Validate connectivity
    validateWaterConnectivity(map);
}

bool FractalGenerator::isNearStartArea(MapData& map, int x, int y, int margin) {
    for (auto& sa : map.getStartingAreas()) {
        int dx = x - sa.centerX;
        int dy = y - sa.centerY;
        if (dx * dx + dy * dy < (sa.radius + margin) * (sa.radius + margin)) {
            return true;
        }
    }
    return false;
}

void FractalGenerator::validateWaterConnectivity(MapData& map) {
    int w = map.getWidth();
    int h = map.getHeight();

    // BFS to find connected land components
    std::vector<int> component(w * h, -1);
    int numComponents = 0;
    std::vector<int> componentSizes;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (map.getTile(x, y) != TileType::Water && component[y * w + x] == -1) {
                // BFS
                std::queue<std::pair<int,int>> q;
                q.push({x, y});
                component[y * w + x] = numComponents;
                int sz = 0;

                while (!q.empty()) {
                    auto [cx, cy] = q.front(); q.pop();
                    sz++;

                    int dx[] = {1, -1, 0, 0};
                    int dy[] = {0, 0, 1, -1};
                    for (int d = 0; d < 4; d++) {
                        int nx = cx + dx[d];
                        int ny = cy + dy[d];
                        if (map.inBounds(nx, ny) && map.getTile(nx, ny) != TileType::Water
                            && component[ny * w + nx] == -1) {
                            component[ny * w + nx] = numComponents;
                            q.push({nx, ny});
                        }
                    }
                }
                componentSizes.push_back(sz);
                numComponents++;
            }
        }
    }

    if (numComponents <= 1) return;

    // Find the largest component
    int largestComp = std::max_element(componentSizes.begin(), componentSizes.end()) - componentSizes.begin();

    // Remove water tiles that separate smaller components from the largest
    // Strategy: for each smaller component, find water tiles adjacent to it and remove them
    // to create a corridor to the largest component
    for (int comp = 0; comp < numComponents; comp++) {
        if (comp == largestComp) continue;

        // Find border water tiles between this component and largest
        // Simple approach: remove water in a line between component centers
        // Find a tile in this component and a tile in the largest
        int sx = -1, sy = -1, tx = -1, ty = -1;
        for (int y = 0; y < h && sx == -1; y++) {
            for (int x = 0; x < w && sx == -1; x++) {
                if (component[y * w + x] == comp) { sx = x; sy = y; }
            }
        }
        for (int y = 0; y < h && tx == -1; y++) {
            for (int x = 0; x < w && tx == -1; x++) {
                if (component[y * w + x] == largestComp) { tx = x; ty = y; }
            }
        }

        if (sx == -1 || tx == -1) continue;

        // Bresenham line and remove water along it with small width
        float dx = (float)(tx - sx);
        float dy = (float)(ty - sy);
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) continue;

        dx /= len; dy /= len;
        float cx = (float)sx, cy = (float)sy;
        int corridorWidth = 3;

        for (float t = 0; t < len; t += 1.0f) {
            int ix = (int)cx;
            int iy = (int)cy;
            for (int oy = -corridorWidth; oy <= corridorWidth; oy++) {
                for (int ox = -corridorWidth; ox <= corridorWidth; ox++) {
                    int nx = ix + ox, ny = iy + oy;
                    if (map.inBounds(nx, ny) && map.getTile(nx, ny) == TileType::Water) {
                        map.setTile(nx, ny, TileType::Ground);
                    }
                }
            }
            cx += dx;
            cy += dy;
        }
    }
}

void FractalGenerator::carveMetalPoint(MapData& map, int cx, int cy, float width, float intensity) {
    int r = (int)std::ceil(width);
    for (int dy = -r; dy <= r; dy++) {
        for (int dx = -r; dx <= r; dx++) {
            float dist = std::sqrt((float)(dx * dx + dy * dy));
            if (dist <= width) {
                int nx = cx + dx;
                int ny = cy + dy;
                if (map.inBounds(nx, ny)) {
                    float falloff = 1.0f - (dist / width);
                    float val = map.getMetal(nx, ny) + intensity * falloff * falloff;
                    map.setMetal(nx, ny, std::min(val, 1.0f));
                }
            }
        }
    }
}

void FractalGenerator::growMetalVein(MapData& map, float x, float y, float angle,
                                      float width, float intensity, int depth, int maxDepth) {
    if (depth >= maxDepth || width < 0.5f) return;

    std::uniform_real_distribution<float> angleDist(-0.4f, 0.4f);
    std::uniform_real_distribution<float> branchProb(0.0f, 1.0f);
    std::uniform_real_distribution<float> branchAngle(0.5f, 1.3f);

    float stepSize = 1.5f;
    int steps = (int)(30.0f / (depth + 1) + 10);

    for (int i = 0; i < steps; i++) {
        int ix = (int)x;
        int iy = (int)y;
        if (!map.inBounds(ix, iy)) break;

        carveMetalPoint(map, ix, iy, width, intensity);

        angle += angleDist(rng_);
        x += std::cos(angle) * stepSize;
        y += std::sin(angle) * stepSize;

        if (branchProb(rng_) < 0.08f && depth < maxDepth - 1) {
            float ba = angle + (branchProb(rng_) > 0.5f ? 1.0f : -1.0f) * branchAngle(rng_);
            growMetalVein(map, x, y, ba, width * 0.65f, intensity * 0.8f, depth + 1, maxDepth);
        }

        width *= 0.995f;
    }
}

void FractalGenerator::generateMetalVeins(MapData& map, float centerX, float centerY,
                                            float radius, float intensity, bool isTangle) {
    int numBranches = isTangle ? 5 : 2;
    int maxDepth = isTangle ? 5 : 3;
    float baseWidth = isTangle ? radius * 0.08f : radius * 0.05f;
    baseWidth = std::max(baseWidth, 1.5f);

    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);
    std::uniform_real_distribution<float> offsetDist(-radius * 0.2f, radius * 0.2f);

    for (int b = 0; b < numBranches; b++) {
        float angle = angleDist(rng_);
        float sx = centerX + offsetDist(rng_);
        float sy = centerY + offsetDist(rng_);
        growMetalVein(map, sx, sy, angle, baseWidth, intensity, 0, maxDepth);
    }
}

void FractalGenerator::generateCommonMetalVeins(MapData& map, const std::vector<StartingArea>& startAreas) {
    int w = map.getWidth();
    int h = map.getHeight();

    std::uniform_real_distribution<float> posDist(0.15f, 0.85f);
    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * M_PI);

    int numVeins = std::max(3, w / 200);
    float baseWidth = std::max(3.0f, w / 200.0f);

    for (int v = 0; v < numVeins; v++) {
        float cx = posDist(rng_) * w;
        float cy = posDist(rng_) * h;

        // Check it's not inside a starting area
        bool inStart = false;
        for (auto& sa : startAreas) {
            float dx = cx - sa.centerX;
            float dy = cy - sa.centerY;
            if (dx * dx + dy * dy < sa.radius * sa.radius) {
                inStart = true;
                break;
            }
        }
        if (inStart) { v--; continue; }

        generateMetalVeins(map, cx, cy, w * 0.15f, 0.9f, true);
    }
}