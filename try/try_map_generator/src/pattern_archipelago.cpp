#include "pattern_archipelago.h"
#include "pathfinding.h"
#include <cmath>
#include <algorithm>
#include <queue>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PatternArchipelago::PatternArchipelago(std::mt19937& rng, PerlinNoise& noise)
    : rng_(rng), noise_(noise), fractal_(rng), placement_(rng, noise) {}

void PatternArchipelago::generate(MapData& map, int numPlayers, PlacementMode placementMode,
                                   const WaterParams& waterParams,
                                   const MetalParams& metalParams) {
    PlacementResult pr = placement_.place(map, numPlayers, placementMode);

    for (auto& sa : pr.areas) {
        map.addStartingArea(sa);
        map.setTile(sa.centerX, sa.centerY, TileType::StartingPoint);
    }

    generateTerrain(map, waterParams);
    ensureConnectivity(map);
    fillVegetation(map);
    placeMetalDeposits(map, metalParams);

    int extraRivers = std::max(0, (int)(waterParams.densityMultiplier * 0.5f));
    if (extraRivers > 0) {
        WaterParams reducedParams = waterParams;
        reducedParams.densityMultiplier *= 0.3f;
        reducedParams.widthMultiplier *= 0.6f;
        fractal_.generateRiverSystem(map, extraRivers, reducedParams);
    }
}

void PatternArchipelago::generateTerrain(MapData& map, const WaterParams& waterParams) {
    int w = map.getWidth();
    int h = map.getHeight();

    float waterThreshold = -0.15f + (waterParams.densityMultiplier - 1.0f) * 0.12f;
    waterThreshold = std::clamp(waterThreshold, -0.4f, 0.2f);

    float noiseScale = 6.0f * waterParams.widthMultiplier;
    noiseScale = std::clamp(noiseScale, 3.0f, 15.0f);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (map.getTile(x, y) == TileType::StartingPoint) continue;

            float nx = (float)x / w;
            float ny = (float)y / h;
            float n = (float)noise_.octaveNoise(nx * noiseScale, ny * noiseScale, 5, 0.5);

            bool nearStart = false;
            for (auto& sa : map.getStartingAreas()) {
                int dx = x - sa.centerX;
                int dy = y - sa.centerY;
                float dist = std::sqrt((float)(dx * dx + dy * dy));
                if (dist < sa.radius * 1.3f) {
                    nearStart = true;
                    break;
                }
            }

            if (!nearStart && n < waterThreshold) {
                map.setTile(x, y, TileType::Water);
            }
        }
    }
}

void PatternArchipelago::ensureConnectivity(MapData& map) {
    auto& areas = map.getStartingAreas();
    int bridgeWidth = std::max(4, map.getWidth() / 200);

    for (size_t i = 0; i < areas.size(); i++) {
        size_t j = (i + 1) % areas.size();

        float dx = (float)(areas[j].centerX - areas[i].centerX);
        float dy = (float)(areas[j].centerY - areas[i].centerY);
        float len = std::sqrt(dx * dx + dy * dy);
        if (len < 1.0f) continue;
        dx /= len;
        dy /= len;

        float cx = (float)areas[i].centerX;
        float cy = (float)areas[i].centerY;

        std::uniform_real_distribution<float> wobble(-0.2f, 0.2f);

        for (float t = 0; t < len; t += 1.0f) {
            int ix = (int)cx;
            int iy = (int)cy;
            for (int oy = -bridgeWidth; oy <= bridgeWidth; oy++) {
                for (int ox = -bridgeWidth; ox <= bridgeWidth; ox++) {
                    if (ox * ox + oy * oy <= bridgeWidth * bridgeWidth) {
                        int nx = ix + ox;
                        int ny = iy + oy;
                        if (map.inBounds(nx, ny) && map.getTile(nx, ny) == TileType::Water) {
                            map.setTile(nx, ny, TileType::Ground);
                        }
                    }
                }
            }
            cx += dx + wobble(rng_) * (-dy);
            cy += dy + wobble(rng_) * dx;
        }
    }

    fractal_.validateWaterConnectivity(map);
}

void PatternArchipelago::fillVegetation(MapData& map) {
    int w = map.getWidth();
    int h = map.getHeight();
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (map.getTile(x, y) != TileType::Ground) continue;

            bool inStartClear = false;
            for (auto& sa : map.getStartingAreas()) {
                int ddx = x - sa.centerX;
                int ddy = y - sa.centerY;
                if (ddx * ddx + ddy * ddy < (sa.radius / 2) * (sa.radius / 2)) {
                    inStartClear = true;
                    break;
                }
            }

            if (!inStartClear) {
                float n = (float)noise_.octaveNoise(x * 0.04, y * 0.04, 3);
                if (n > -0.1f && prob(rng_) < 0.4f) {
                    map.setTile(x, y, TileType::Tree);
                }
                if (n < -0.3f && prob(rng_) < 0.1f) {
                    map.setTile(x, y, TileType::Rock);
                }
            }
        }
    }
}

void PatternArchipelago::placeMetalDeposits(MapData& map, const MetalParams& params) {
    for (auto& sa : map.getStartingAreas()) {
        fractal_.generateMetalVeins(map, (float)sa.centerX, (float)sa.centerY,
                                     (float)sa.radius * 0.8f, 0.8f, true, params);
    }
    fractal_.generateCommonMetalVeins(map, map.getStartingAreas(), params);
}