#include "pattern_star.h"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PatternStar::PatternStar(std::mt19937& rng, PerlinNoise& noise)
    : rng_(rng), noise_(noise), fractal_(rng), placement_(rng, noise) {}

void PatternStar::generate(MapData& map, int numPlayers, PlacementMode placementMode,
                            const WaterParams& waterParams,
                            const MetalParams& metalParams) {
    PlacementResult pr = placement_.place(map, numPlayers, placementMode);

    for (auto& sa : pr.areas) {
        map.addStartingArea(sa);
        map.setTile(sa.centerX, sa.centerY, TileType::StartingPoint);

        int clearRadius = sa.radius / 2;
        for (int dy = -clearRadius; dy <= clearRadius; dy++) {
            for (int dx = -clearRadius; dx <= clearRadius; dx++) {
                if (dx * dx + dy * dy <= clearRadius * clearRadius) {
                    int nx = sa.centerX + dx;
                    int ny = sa.centerY + dy;
                    if (map.inBounds(nx, ny) && map.getTile(nx, ny) != TileType::StartingPoint) {
                        map.setTile(nx, ny, TileType::Ground);
                    }
                }
            }
        }
    }

    fillForest(map);
    carvePaths(map);
    placeMetalDeposits(map, metalParams);

    int riverCount = std::max(1, map.getWidth() / 500);
    fractal_.generateRiverSystem(map, riverCount, waterParams);
}

void PatternStar::carvePath(MapData& map, int x1, int y1, int x2, int y2, int width) {
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    float len = std::sqrt(dx * dx + dy * dy);
    if (len < 1.0f) return;
    dx /= len;
    dy /= len;

    std::uniform_real_distribution<float> wobble(-0.3f, 0.3f);

    float perpX = -dy, perpY = dx;
    float cx = (float)x1, cy = (float)y1;

    for (float t = 0; t < len; t += 1.0f) {
        float w = wobble(rng_);
        float px = cx + perpX * w * width;
        float py = cy + perpY * w * width;

        int ix = (int)px;
        int iy = (int)py;

        for (int oy = -width; oy <= width; oy++) {
            for (int ox = -width; ox <= width; ox++) {
                if (ox * ox + oy * oy <= width * width) {
                    int nx = ix + ox;
                    int ny = iy + oy;
                    if (map.inBounds(nx, ny)) {
                        TileType tile = map.getTile(nx, ny);
                        if (tile == TileType::Tree || tile == TileType::Rock) {
                            map.setTile(nx, ny, TileType::Ground);
                        }
                    }
                }
            }
        }

        cx += dx;
        cy += dy;
    }
}

void PatternStar::carvePaths(MapData& map) {
    int w = map.getWidth();
    int h = map.getHeight();
    int centerX = w / 2;
    int centerY = h / 2;

    int pathWidth = std::max(3, w / 150);
    auto& areas = map.getStartingAreas();

    // Star: all paths to center
    for (auto& sa : areas) {
        carvePath(map, sa.centerX, sa.centerY, centerX, centerY, pathWidth);
    }

    // Clear center area
    int centerClear = std::max(15, w / 30);
    for (int dy = -centerClear; dy <= centerClear; dy++) {
        for (int dx = -centerClear; dx <= centerClear; dx++) {
            if (dx * dx + dy * dy <= centerClear * centerClear) {
                int nx = centerX + dx;
                int ny = centerY + dy;
                if (map.inBounds(nx, ny)) {
                    TileType tile = map.getTile(nx, ny);
                    if (tile == TileType::Tree || tile == TileType::Rock) {
                        map.setTile(nx, ny, TileType::Ground);
                    }
                }
            }
        }
    }

    // Adjacent player paths
    for (size_t i = 0; i < areas.size(); i++) {
        size_t j = (i + 1) % areas.size();
        int mx = (areas[i].centerX + areas[j].centerX) / 2;
        int my = (areas[i].centerY + areas[j].centerY) / 2;
        carvePath(map, areas[i].centerX, areas[i].centerY, mx, my, pathWidth - 1);
        carvePath(map, mx, my, areas[j].centerX, areas[j].centerY, pathWidth - 1);
    }
}

void PatternStar::fillForest(MapData& map) {
    int w = map.getWidth();
    int h = map.getHeight();
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (map.getTile(x, y) != TileType::Ground) continue;

            bool inStartClear = false;
            for (auto& sa : map.getStartingAreas()) {
                int dx = x - sa.centerX;
                int dy = y - sa.centerY;
                if (dx * dx + dy * dy < (sa.radius / 2) * (sa.radius / 2)) {
                    inStartClear = true;
                    break;
                }
            }

            if (!inStartClear) {
                float n = (float)noise_.octaveNoise(x * 0.02, y * 0.02, 4);
                if (n > -0.3f && prob(rng_) < 0.5f) {
                    map.setTile(x, y, TileType::Tree);
                }
            }
        }
    }
}

void PatternStar::placeMetalDeposits(MapData& map, const MetalParams& params) {
    for (auto& sa : map.getStartingAreas()) {
        fractal_.generateMetalVeins(map, (float)sa.centerX, (float)sa.centerY,
                                     (float)sa.radius * 0.8f, 0.8f, true, params);
    }
    fractal_.generateCommonMetalVeins(map, map.getStartingAreas(), params);
}