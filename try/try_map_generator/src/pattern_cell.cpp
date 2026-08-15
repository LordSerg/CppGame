#include "pattern_cell.h"
#include <cmath>
#include <algorithm>
#include <set>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

PatternCell::PatternCell(std::mt19937& rng, PerlinNoise& noise)
    : rng_(rng), noise_(noise), fractal_(rng), placement_(rng, noise) {}

bool PatternCell::isInsideStartArea(const StartingArea& sa, int x, int y) const {
    if (sa.hasShape()) {
        // Check shape tiles - use squared distance as quick reject first
        int dx = x - sa.centerX;
        int dy = y - sa.centerY;
        if (dx * dx + dy * dy > (int)(sa.radius * sa.radius * 2.5f)) return false;

        for (auto& [tx, ty] : sa.shapeTiles) {
            if (tx == x && ty == y) return true;
        }
        return false;
    } else {
        int dx = x - sa.centerX;
        int dy = y - sa.centerY;
        return dx * dx + dy * dy <= sa.radius * sa.radius;
    }
}

bool PatternCell::isInsideAnyStartArea(const MapData& map, int x, int y) const {
    for (auto& sa : map.getStartingAreas()) {
        if (isInsideStartArea(sa, x, y)) return true;
    }
    return false;
}

void PatternCell::generate(MapData& map, int numPlayers, PlacementMode placementMode,
                            const WaterParams& waterParams,
                            const MetalParams& metalParams) {
    // Place starting areas using selected strategy
    PlacementResult pr = placement_.place(map, numPlayers, placementMode);

    for (auto& sa : pr.areas) {
        map.addStartingArea(sa);
        map.setTile(sa.centerX, sa.centerY, TileType::StartingPoint);

        // Clear space around starting point
        int clearRadius = sa.radius / 3;
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

    buildWalls(map);
    fillStartingAreaTrees(map);
    fillCommonAreaForest(map);
    placeMetalDeposits(map, metalParams);

    int riverCount = std::max(1, map.getWidth() / 400);
    fractal_.generateRiverSystem(map, riverCount, waterParams);
}

void PatternCell::buildWalls(MapData& map) {
    for (auto& sa : map.getStartingAreas()) {
        if (sa.hasShape()) {
            // Use pre-computed boundary tiles for shaped areas
            for (auto& [bx, by] : sa.boundaryTiles) {
                if (map.inBounds(bx, by) && map.getTile(bx, by) == TileType::Ground) {
                    map.setTile(bx, by, TileType::Rock);
                }
            }
        } else {
            // Circle wall (original behavior)
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
}

void PatternCell::fillStartingAreaTrees(MapData& map) {
    std::uniform_real_distribution<float> treeDist(0.0f, 1.0f);

    for (auto& sa : map.getStartingAreas()) {
        int clearRadius = sa.radius / 3;

        if (sa.hasShape()) {
            // Place trees inside shape but outside clear zone and boundary
            std::set<std::pair<int,int>> boundarySet(sa.boundaryTiles.begin(), sa.boundaryTiles.end());

            for (auto& [tx, ty] : sa.shapeTiles) {
                int dx = tx - sa.centerX;
                int dy = ty - sa.centerY;
                float dist = std::sqrt((float)(dx * dx + dy * dy));

                if (dist > clearRadius && boundarySet.find({tx, ty}) == boundarySet.end()) {
                    if (map.inBounds(tx, ty) && map.getTile(tx, ty) == TileType::Ground) {
                        float n = (float)noise_.octaveNoise(tx * 0.05, ty * 0.05, 3);
                        if (n > -0.1f && treeDist(rng_) < 0.4f) {
                            map.setTile(tx, ty, TileType::Tree);
                        }
                    }
                }
            }
        } else {
            // Original circular behavior
            int innerRadius = sa.radius - std::max(4, sa.radius / 8) - 2;

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
}

void PatternCell::fillCommonAreaForest(MapData& map) {
    int w = map.getWidth();
    int h = map.getHeight();
    std::uniform_real_distribution<float> prob(0.0f, 1.0f);

    // Build a fast lookup for shaped areas
    // For large maps, we'll use bounding-box checks + distance
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            if (map.getTile(x, y) != TileType::Ground) continue;

            bool inStart = false;
            for (auto& sa : map.getStartingAreas()) {
                int dx = x - sa.centerX;
                int dy = y - sa.centerY;
                float expandedR = sa.radius + 10.0f;
                if (dx * dx + dy * dy < expandedR * expandedR) {
                    // Could be inside - for circle this is definitive
                    // For shaped areas, check if tile is in the shape
                    if (!sa.hasShape()) {
                        inStart = true;
                    } else {
                        // Quick: if it's near enough, consider it "in start area" to avoid dense forest
                        // right at the walls
                        inStart = true;
                    }
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

void PatternCell::placeMetalDeposits(MapData& map, const MetalParams& params) {
    for (auto& sa : map.getStartingAreas()) {
        fractal_.generateMetalVeins(map, (float)sa.centerX, (float)sa.centerY,
                                     (float)sa.radius * 0.8f, 0.8f, true, params);
    }
    fractal_.generateCommonMetalVeins(map, map.getStartingAreas(), params);
}