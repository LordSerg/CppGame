#include "pattern_cell.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PatternCell::PatternCell(std::mt19937& rng, PerlinNoise& noise)
    : rng_(rng), noise_(noise), fractal_(rng) {}

void PatternCell::generate(MapData& map, int numPlayers) {
    placeStartingAreas(map, numPlayers);
    buildRockWalls(map);
    fillStartingAreaTrees(map);
    fillCommonAreaForest(map);
    placeMetalDeposits(map);

    // Generate rivers in common area
    int riverCount = std::max(1, map.getWidth() / 500);
    fractal_.generateRiverSystem(map, riverCount);
}

void PatternCell::placeStartingAreas(MapData& map, int numPlayers) {
    int w = map.getWidth();
    int h = map.getHeight();

    float centerX = w / 2.0f;
    float centerY = h / 2.0f;
    float radius = std::min(w, h) * 0.35f;
    int areaRadius = (int)(std::min(w, h) * 0.08f);
    areaRadius = std::max(areaRadius, 30);

    for (int i = 0; i < numPlayers; i++) {
        float angle = 2.0f * M_PI * i / numPlayers - M_PI / 2.0f;
        int cx = (int)(centerX + radius * std::cos(angle));
        int cy = (int)(centerY + radius * std::sin(angle));

        cx = std::clamp(cx, areaRadius + 10, w - areaRadius - 10);
        cy = std::clamp(cy, areaRadius + 10, h - areaRadius - 10);

        StartingArea sa;
        sa.centerX = cx;
        sa.centerY = cy;
        sa.radius = areaRadius;
        sa.playerIndex = i;
        map.addStartingArea(sa);

        // Mark starting point
        map.setTile(cx, cy, TileType::StartingPoint);

        // Clear starting area
        int clearRadius = areaRadius / 3;
        for (int dy = -clearRadius; dy <= clearRadius; dy++) {
            for (int dx = -clearRadius; dx <= clearRadius; dx++) {
                if (dx * dx + dy * dy <= clearRadius * clearRadius) {
                    int nx = cx + dx;
                    int ny = cy + dy;
                    if (map.inBounds(nx, ny)) {
                        map.setTile(nx, ny, TileType::Ground);
                    }
                }
            }
        }
    }
}

void PatternCell::buildRockWalls(MapData& map) {
    for (auto& sa : map.getStartingAreas()) {
        int wallRadius = sa.radius;
        int wallThickness = std::max(3, sa.radius / 8);

        for (int dy = -(wallRadius + wallThickness); dy <= wallRadius + wallThickness; dy++) {
            for (int dx = -(wallRadius + wallThickness); dx <= wallRadius + wallThickness; dx++) {
                float dist = std::sqrt((float)(dx * dx + dy * dy));
                if (dist >= wallRadius - 1 && dist <= wallRadius + wallThickness) {
                    int nx = sa.centerX + dx;
                    int ny = sa.centerY + dy;
                    if (map.inBounds(nx, ny) && map.getTile(nx, ny) == TileType::Ground) {
                        map.setTile(nx, ny, TileType::Rock);
                    }
                }
            }
        }
    }
}

void PatternCell::fillStartingAreaTrees(MapData& map) {
    std::uniform_real_distribution<float> treeDist(0.0f, 1.0f);

    for (auto& sa : map.getStartingAreas()) {
        int innerRadius = sa.radius - std::max(4, sa.radius / 8) - 2;
        int clearRadius = sa.radius / 3;

        for (int dy = -innerRadius; dy <= innerRadius; dy++) {
            for (int dx = -innerRadius; dx <= innerRadius; dx++) {
                float dist = std::sqrt((float)(dx * dx + dy * dy));
                if (dist > clearRadius && dist < innerRadius) {
                    int nx = sa.centerX + dx;
                    int ny = sa.centerY + dy;
                    if (map.inBounds(nx, ny) && map.getTile(nx, ny) == TileType::Ground) {
                        float n = (float)noise_.octaveNoise(nx * 0.05, ny * 0.05, 3);
                        if (n > -0.1f && treeDist(rng_) < 0.4f) {
                            map.setTile(nx, ny, TileType::Tree);
                        }
                    }
                }
            }
        }
    }
}

void PatternCell::fillCommonAreaForest(MapData& map) {
    int w = map.getWidth();
    int h = map.getHeight();
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (map.getTile(x, y) != TileType::Ground) continue;

            // Check if in any starting area
            bool inStart = false;
            for (auto& sa : map.getStartingAreas()) {
                int dx = x - sa.centerX;
                int dy = y - sa.centerY;
                if (dx * dx + dy * dy < (sa.radius + 10) * (sa.radius + 10)) {
                    inStart = true;
                    break;
                }
            }

            if (!inStart) {
                float n = (float)noise_.octaveNoise(x * 0.03, y * 0.03, 4);
                if (n > -0.2f && prob(rng_) < 0.55f) {
                    map.setTile(x, y, TileType::Tree);
                }
            }
        }
    }
}

void PatternCell::placeMetalDeposits(MapData& map) {
    // Metal in each starting area
    for (auto& sa : map.getStartingAreas()) {
        fractal_.generateMetalVeins(map, (float)sa.centerX, (float)sa.centerY,
                                     (float)sa.radius * 0.8f, 0.8f, true);
    }

    // Rich metal in common area
    fractal_.generateCommonMetalVeins(map, map.getStartingAreas());
}