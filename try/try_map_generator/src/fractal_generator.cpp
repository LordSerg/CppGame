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
                                        float width, float widthMul, int depth, int maxDepth,
                                        float branchChance,
                                        std::vector<std::pair<int,int>>& waterTiles) {
    if (depth >= maxDepth || width < 0.8f) return;

    std::uniform_real_distribution<float> angleDist(-0.3f, 0.3f);
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    std::uniform_real_distribution<float> branchAngle(0.4f, 1.2f);

    float stepSize = 2.0f;
    int steps = (int)(std::max(25.0f, (float)map.getWidth() * 0.18f / (depth + 1)));

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

        // Branch probability - scaled by branchChance
        if (prob(rng_) < branchChance && depth < maxDepth - 1) {
            float ba = angle + (prob(rng_) > 0.5f ? 1.0f : -1.0f) * branchAngle(rng_);
            growRiverBranch(map, x, y, ba, width * 0.55f, widthMul,
                            depth + 1, maxDepth, branchChance * 0.8f, waterTiles);
        }

        // Secondary smaller branch
        if (prob(rng_) < branchChance * 0.5f && depth < maxDepth - 2) {
            float ba = angle + (prob(rng_) > 0.5f ? 1.0f : -1.0f) * branchAngle(rng_) * 1.3f;
            growRiverBranch(map, x, y, ba, width * 0.4f, widthMul,
                            depth + 2, maxDepth, branchChance * 0.5f, waterTiles);
        }

        // Gradually reduce width
        width *= 0.997f;
    }
}

void FractalGenerator::generateRiverSystem(MapData& map, int numBaseSources,
                                            const WaterParams& params) {
    int w = map.getWidth();
    int h = map.getHeight();
    std::vector<std::pair<int,int>> waterTiles;

    std::uniform_int_distribution<int> edgeDist(0, 3);
    std::uniform_real_distribution<float> posDist(0.05f, 0.95f);
    std::uniform_real_distribution<float> angleDist(-0.5f, 0.5f);

    float baseWidth = std::max(2.0f, w / 150.0f) * params.widthMultiplier;
    int maxDepth = 5;

    int totalSources = (int)(numBaseSources * params.densityMultiplier) + params.extraSources;
    totalSources = std::max(1, totalSources);

    // Also add some interior river sources for more coverage
    int interiorSources = std::max(0, totalSources / 3);
    int edgeSources = totalSources - interiorSources;

    // Edge-originating rivers
    for (int s = 0; s < edgeSources; s++) {
        float sx, sy, angle;
        int edge = edgeDist(rng_);

        switch (edge) {
            case 0: // top
                sx = posDist(rng_) * w;
                sy = 0;
                angle = (float)M_PI / 2.0f;
                break;
            case 1: // bottom
                sx = posDist(rng_) * w;
                sy = (float)(h - 1);
                angle = -(float)M_PI / 2.0f;
                break;
            case 2: // left
                sx = 0;
                sy = posDist(rng_) * h;
                angle = 0;
                break;
            default: // right
                sx = (float)(w - 1);
                sy = posDist(rng_) * h;
                angle = (float)M_PI;
                break;
        }

        angle += angleDist(rng_);
        float branchChance = 0.03f * params.densityMultiplier;
        growRiverBranch(map, sx, sy, angle, baseWidth, params.widthMultiplier,
                        0, maxDepth, branchChance, waterTiles);
    }

    // Interior-originating rivers (flow outward from random interior points)
    std::uniform_real_distribution<float> interiorPos(0.2f, 0.8f);
    std::uniform_real_distribution<float> fullAngle(0.0f, 2.0f * (float)M_PI);
    for (int s = 0; s < interiorSources; s++) {
        float sx = interiorPos(rng_) * w;
        float sy = interiorPos(rng_) * h;

        // Check not inside a starting area
        bool inStart = false;
        for (auto& sa : map.getStartingAreas()) {
            float dx = sx - sa.centerX;
            float dy = sy - sa.centerY;
            if (dx * dx + dy * dy < (sa.radius + 20) * (sa.radius + 20)) {
                inStart = true;
                break;
            }
        }
        if (inStart) continue;

        float angle = fullAngle(rng_);
        float interiorWidth = baseWidth * 0.7f;
        float branchChance = 0.04f * params.densityMultiplier;
        growRiverBranch(map, sx, sy, angle, interiorWidth, params.widthMultiplier,
                        0, maxDepth, branchChance, waterTiles);

        // Second branch going roughly opposite direction
        float angle2 = angle + (float)M_PI + angleDist(rng_);
        growRiverBranch(map, sx, sy, angle2, interiorWidth * 0.8f, params.widthMultiplier,
                        1, maxDepth, branchChance * 0.7f, waterTiles);
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
                std::queue<std::pair<int,int>> q;
                q.push({x, y});
                component[y * w + x] = numComponents;
                int sz = 0;

                while (!q.empty()) {
                    auto [cx, cy] = q.front(); q.pop();
                    sz++;

                    int ddx[] = {1, -1, 0, 0};
                    int ddy[] = {0, 0, 1, -1};
                    for (int d = 0; d < 4; d++) {
                        int nx = cx + ddx[d];
                        int ny = cy + ddy[d];
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
    int largestComp = (int)(std::max_element(componentSizes.begin(), componentSizes.end()) - componentSizes.begin());

    // For each smaller component, carve a corridor to the largest
    // Use a smarter approach: find the closest pair of tiles between components
    for (int comp = 0; comp < numComponents; comp++) {
        if (comp == largestComp) continue;
        if (componentSizes[comp] < 4) {
            // Tiny isolated patch - just convert to water
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    if (component[y * w + x] == comp) {
                        if (map.getTile(x, y) != TileType::StartingPoint)
                            map.setTile(x, y, TileType::Water);
                    }
                }
            }
            continue;
        }

        // Find a tile in this component and nearest tile in largest component
        // Sample approach: find centroid of small comp, then BFS outward through water to find largest
        int sx = -1, sy = -1;
        long long sumX = 0, sumY = 0;
        int cnt = 0;
        for (int y = 0; y < h && sx == -1; y++) {
            for (int x = 0; x < w && sx == -1; x++) {
                if (component[y * w + x] == comp) {
                    sumX += x; sumY += y; cnt++;
                    if (sx == -1) { sx = x; sy = y; }
                }
            }
        }

        // Find nearest tile in largest component to centroid
        int centX = (int)(sumX / cnt);
        int centY = (int)(sumY / cnt);
        int tx = -1, ty = -1;
        float bestDist = 1e18f;
        // Sample instead of checking all
        int sampleStep = std::max(1, w / 100);
        for (int y = 0; y < h; y += sampleStep) {
            for (int x = 0; x < w; x += sampleStep) {
                if (component[y * w + x] == largestComp) {
                    float dd = (float)((x - centX) * (x - centX) + (y - centY) * (y - centY));
                    if (dd < bestDist) {
                        bestDist = dd;
                        tx = x; ty = y;
                    }
                }
            }
        }

        if (tx == -1) continue;

        // Carve corridor from (sx, sy) toward (tx, ty)
        float dx = (float)(tx - sx);
        float dy = (float)(ty - sy);
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) continue;

        dx /= len; dy /= len;
        float cx = (float)sx, cy = (float)sy;
        int corridorWidth = std::max(3, w / 300);

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
                                      float width, float intensity, float widthMul,
                                      int depth, int maxDepth) {
    if (depth >= maxDepth || width < 0.4f) return;

    std::uniform_real_distribution<float> angleDist(-0.4f, 0.4f);
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);
    std::uniform_real_distribution<float> branchAngle(0.5f, 1.3f);

    float stepSize = 1.5f;
    int steps = (int)(35.0f / (depth + 1) + 12);

    for (int i = 0; i < steps; i++) {
        int ix = (int)x;
        int iy = (int)y;
        if (!map.inBounds(ix, iy)) break;

        carveMetalPoint(map, ix, iy, width, intensity);

        angle += angleDist(rng_);
        x += std::cos(angle) * stepSize;
        y += std::sin(angle) * stepSize;

        // Main branch
        if (prob(rng_) < 0.1f && depth < maxDepth - 1) {
            float ba = angle + (prob(rng_) > 0.5f ? 1.0f : -1.0f) * branchAngle(rng_);
            growMetalVein(map, x, y, ba, width * 0.6f, intensity * 0.8f,
                          widthMul, depth + 1, maxDepth);
        }

        // Thin secondary tendrils
        if (prob(rng_) < 0.05f && depth < maxDepth - 2) {
            float ba = angle + (prob(rng_) > 0.5f ? 1.0f : -1.0f) * branchAngle(rng_) * 1.5f;
            growMetalVein(map, x, y, ba, width * 0.35f, intensity * 0.6f,
                          widthMul, depth + 2, maxDepth);
        }

        width *= 0.994f;
    }
}

void FractalGenerator::generateMetalVeins(MapData& map, float centerX, float centerY,
                                            float radius, float intensity, bool isTangle,
                                            const MetalParams& params) {
    int numBranches = isTangle ? 6 : 3;
    numBranches = (int)(numBranches * params.densityMultiplier);
    numBranches = std::max(1, numBranches);

    int maxDepth = isTangle ? 6 : 4;
    float baseWidth = isTangle ? radius * 0.1f : radius * 0.06f;
    baseWidth = std::max(baseWidth, 1.5f);
    baseWidth *= params.widthMultiplier;

    float effectiveIntensity = intensity * params.intensityMultiplier;
    effectiveIntensity = std::min(effectiveIntensity, 1.0f);

    std::uniform_real_distribution<float> angleDist(0.0f, 2.0f * (float)M_PI);
    std::uniform_real_distribution<float> offsetDist(-radius * 0.25f, radius * 0.25f);

    for (int b = 0; b < numBranches; b++) {
        float angle = angleDist(rng_);
        float sx = centerX + offsetDist(rng_);
        float sy = centerY + offsetDist(rng_);
        growMetalVein(map, sx, sy, angle, baseWidth, effectiveIntensity,
                      params.widthMultiplier, 0, maxDepth);
    }
}

void FractalGenerator::generateCommonMetalVeins(MapData& map,
                                                  const std::vector<StartingArea>& startAreas,
                                                  const MetalParams& params) {
    int w = map.getWidth();
    int h = map.getHeight();

    std::uniform_real_distribution<float> posDist(0.1f, 0.9f);

    int numVeins = std::max(3, w / 180);
    numVeins = (int)(numVeins * params.densityMultiplier) + params.extraVeins;
    numVeins = std::max(2, numVeins);

    int attempts = 0;
    int placed = 0;
    while (placed < numVeins && attempts < numVeins * 5) {
        attempts++;
        float cx = posDist(rng_) * w;
        float cy = posDist(rng_) * h;

        // Check it's not inside a starting area
        bool inStart = false;
        for (auto& sa : startAreas) {
            float dx = cx - sa.centerX;
            float dy = cy - sa.centerY;
            if (dx * dx + dy * dy < (float)(sa.radius * sa.radius)) {
                inStart = true;
                break;
            }
        }
        if (inStart) continue;

        generateMetalVeins(map, cx, cy, w * 0.12f, 0.9f, true, params);
        placed++;
    }

    // Add some long linear veins crossing the map for variety
    std::uniform_real_distribution<float> fullAngle(0.0f, 2.0f * (float)M_PI);
    int longVeins = std::max(1, (int)(params.densityMultiplier * 2));
    for (int v = 0; v < longVeins; v++) {
        float sx = posDist(rng_) * w;
        float sy = posDist(rng_) * h;
        float angle = fullAngle(rng_);
        float veinWidth = std::max(1.5f, w / 300.0f) * params.widthMultiplier;
        growMetalVein(map, sx, sy, angle, veinWidth, 0.7f * params.intensityMultiplier,
                      params.widthMultiplier, 0, 4);
    }
}