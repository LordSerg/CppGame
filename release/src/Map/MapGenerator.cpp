#include "MapGenerator.h"
#include "Map.h"
#include "Tile.h"
#include "../Entities/Peasant.h"
#include "../Entities/Building.h"
#include "../Entities/Obstacle.h"
#include <algorithm>
#include <cmath>
#include <queue>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <iostream>
#include <numeric>
#include <cassert>

// ============================================================
// PerlinNoise Implementation
// ============================================================

PerlinNoise::PerlinNoise(uint32_t seed) {
    permutation.resize(256);
    std::iota(permutation.begin(), permutation.end(), 0);
    std::mt19937 engine(seed);
    std::shuffle(permutation.begin(), permutation.end(), engine);
    // Duplicate the permutation table
    permutation.insert(permutation.end(), permutation.begin(), permutation.end());
}

double PerlinNoise::Fade(double t) const {
    return t * t * t * (t * (t * 6.0 - 15.0) + 10.0);
}

double PerlinNoise::Lerp(double t, double a, double b) const {
    return a + t * (b - a);
}

double PerlinNoise::Grad(int hash, double x, double y) const {
    int h = hash & 7;
    double u = h < 4 ? x : y;
    double v = h < 4 ? y : x;
    return ((h & 1) ? -u : u) + ((h & 2) ? -2.0 * v : 2.0 * v);
}

double PerlinNoise::Noise(double x, double y) const {
    int X = static_cast<int>(std::floor(x)) & 255;
    int Y = static_cast<int>(std::floor(y)) & 255;

    x -= std::floor(x);
    y -= std::floor(y);

    double u = Fade(x);
    double v = Fade(y);

    int A  = permutation[X] + Y;
    int AA = permutation[A];
    int AB = permutation[A + 1];
    int B  = permutation[X + 1] + Y;
    int BA = permutation[B];
    int BB = permutation[B + 1];

    double res = Lerp(v,
        Lerp(u, Grad(permutation[AA], x, y),
                Grad(permutation[BA], x - 1, y)),
        Lerp(u, Grad(permutation[AB], x, y - 1),
                Grad(permutation[BB], x - 1, y - 1)));

    // Normalize to [-1, 1]
    return res / 1.5;
}

double PerlinNoise::FBM(double x, double y, int octaves, double lacunarity,
                         double persistence) const {
    double total = 0.0;
    double amplitude = 1.0;
    double frequency = 1.0;
    double maxAmplitude = 0.0;

    for (int i = 0; i < octaves; i++) {
        total += Noise(x * frequency, y * frequency) * amplitude;
        maxAmplitude += amplitude;
        amplitude *= persistence;
        frequency *= lacunarity;
    }

    return total / maxAmplitude;
}

// ============================================================
// MapGenConfig
// ============================================================

MapGenConfig MapGenConfig::GetDefault(MapSize size, int numPlayers) {
    MapGenConfig cfg;
    int dim = static_cast<int>(size);
    cfg.width = dim;
    cfg.height = dim;
    cfg.seed = 0; // Will be overridden
    cfg.numPlayers = numPlayers;

    // Scale parameters based on map size
    double scaleFactor = dim / 1000.0; // Normalize to medium map

    cfg.terrainScale = 0.003 / scaleFactor * 2.0;
    cfg.forestScale = 0.008 / scaleFactor * 2.0;
    cfg.forestDensity = 0.45;
    cfg.treeDensity = 0.35;

    cfg.numRivers = std::max(2, static_cast<int>(3 * scaleFactor));
    cfg.numLakes = std::max(3, static_cast<int>(5 * scaleFactor));
    cfg.lakeMinRadius = 5.0 * scaleFactor;
    cfg.lakeMaxRadius = 20.0 * scaleFactor;
    cfg.riverWidth = 2.0 + scaleFactor;

    cfg.startingZoneRadius = static_cast<int>(25 * scaleFactor);
    cfg.startingResourceRadius = static_cast<int>(35 * scaleFactor);
    cfg.minTreesPerStart = 20;
    cfg.minStonesPerStart = 8;
    cfg.waterAccessMaxDist = static_cast<int>(50 * scaleFactor);

    cfg.pathWidth = 2.0 + scaleFactor * 0.5;
    cfg.numMainPaths = std::max(3, static_cast<int>(4 * scaleFactor));

    cfg.minPlayerDistance = static_cast<int>(dim * 0.3);

    return cfg;
}

// ============================================================
// MapGenerator
// ============================================================

MapGenerator::MapGenerator(uint32_t seed) : seed(seed), rng(seed),
    mapWidth(0), mapHeight(0) {
    if (seed == 0) {
        std::random_device rd;
        this->seed = rd();
        rng.seed(this->seed);
    }
}

void MapGenerator::SetSeed(uint32_t newSeed) {
    seed = newSeed;
    rng.seed(seed);
}

std::vector<StartingZone> MapGenerator::Generate(Map* map, int numPlayers,
                                                   MapSize size) {
    config = MapGenConfig::GetDefault(size, numPlayers);
    config.seed = seed;
    config.numPlayers = numPlayers;

    mapWidth = config.width;
    mapHeight = config.height;

    std::cout << "[MapGen] Generating " << mapWidth << "x" << mapHeight
              << " map for " << numPlayers << " players, seed=" << seed
              << std::endl;

    // Create noise generators with different seeds for variety
    PerlinNoise terrainNoise(seed);
    PerlinNoise moistureNoise(seed + 1);
    PerlinNoise forestNoise(seed + 2);
    PerlinNoise lakeNoise(seed + 3);
    PerlinNoise stoneNoise(seed + 4);

    // Phase 1: Initialize buffers
    std::cout << "[MapGen] Phase 1: Initializing buffers..." << std::endl;
    InitializeBuffers();

    // Phase 2: Generate base terrain height
    std::cout << "[MapGen] Phase 2: Generating height map..." << std::endl;
    GenerateHeightMap(terrainNoise);

    // Phase 3: Generate moisture map (affects where forests grow)
    std::cout << "[MapGen] Phase 3: Generating moisture map..." << std::endl;
    GenerateMoistureMap(moistureNoise);

    // Phase 4: Generate forest density map
    std::cout << "[MapGen] Phase 4: Generating forest map..." << std::endl;
    GenerateForestMap(forestNoise);

    // Phase 5: Generate water features
    std::cout << "[MapGen] Phase 5a: Generating lakes..." << std::endl;
    GenerateLakes(lakeNoise);

    std::cout << "[MapGen] Phase 5b: Generating rivers..." << std::endl;
    GenerateRivers();

    std::cout << "[MapGen] Phase 5c: Smoothing water edges..." << std::endl;
    SmoothWaterEdges();

    // Phase 6: Place starting zones
    std::cout << "[MapGen] Phase 6: Placing starting zones..." << std::endl;
    std::vector<StartingZone> zones = PlaceStartingZones(numPlayers);

    // Phase 7: Generate paths connecting starting zones and through forests
    std::cout << "[MapGen] Phase 7: Generating paths..." << std::endl;
    GenerateMainPaths(zones);

    // Phase 8: Place trees and stones
    std::cout << "[MapGen] Phase 8a: Placing trees..." << std::endl;
    PlaceTrees();

    std::cout << "[MapGen] Phase 8b: Placing stones..." << std::endl;
    PlaceStones(stoneNoise);

    // Phase 9: Ensure starting resources
    std::cout << "[MapGen] Phase 9: Ensuring starting resources..." << std::endl;
    for (auto& zone : zones) {
        PlaceStartingResources(zone);
        EnsureWaterAccess(zone);
    }

    // Phase 10: Verify connectivity
    std::cout << "[MapGen] Phase 10: Verifying connectivity..." << std::endl;
    if (!VerifyAllStartsConnected(zones)) {
        std::cout << "[MapGen] WARNING: Not all starts connected, "
                  << "adding emergency paths..." << std::endl;
        // Force paths between all starts
        for (size_t i = 0; i < zones.size(); i++) {
            for (size_t j = i + 1; j < zones.size(); j++) {
                CarvePathBetween(zones[i].center, zones[j].center,
                                 config.pathWidth * 2.0);
            }
        }
    }

    // Phase 11: Apply to map
    std::cout << "[MapGen] Phase 11: Applying to map..." << std::endl;
    ApplyToMap(map, zones);

    std::cout << "[MapGen] Generation complete!" << std::endl;
    return zones;
}

// ============================================================
// Buffer Initialization
// ============================================================

void MapGenerator::InitializeBuffers() {
    heightMap.assign(mapWidth, std::vector<float>(mapHeight, 0.0f));
    moistureMap.assign(mapWidth, std::vector<float>(mapHeight, 0.0f));
    forestMap.assign(mapWidth, std::vector<float>(mapHeight, 0.0f));
    waterMask.assign(mapWidth, std::vector<bool>(mapHeight, false));
    treeMask.assign(mapWidth, std::vector<bool>(mapHeight, false));
    stoneMask.assign(mapWidth, std::vector<bool>(mapHeight, false));
    pathMask.assign(mapWidth, std::vector<bool>(mapHeight, false));
}

// ============================================================
// Height Map Generation
// ============================================================

void MapGenerator::GenerateHeightMap(PerlinNoise& noise) {
    for (int x = 0; x < mapWidth; x++) {
        for (int y = 0; y < mapHeight; y++) {
            double nx = x * config.terrainScale;
            double ny = y * config.terrainScale;

            // Multi-octave noise for natural terrain
            double h = noise.FBM(nx, ny, 6, 2.0, 0.5);

            // Normalize from [-1,1] to [0,1]
            h = (h + 1.0) * 0.5;

            // Apply island mask - lower terrain near edges to prevent
            // water from dominating edges (optional, makes nicer maps)
            double edgeX = (double)x / mapWidth;
            double edgeY = (double)y / mapHeight;
            double edgeDist = std::min({edgeX, 1.0 - edgeX, edgeY,
                                         1.0 - edgeY});
            double edgeFactor = std::min(1.0, edgeDist * 8.0);
            // Subtle edge influence, don't make it too island-like
            h = h * (0.7 + 0.3 * edgeFactor);

            heightMap[x][y] = static_cast<float>(h);
        }
    }
}

// ============================================================
// Moisture Map
// ============================================================

void MapGenerator::GenerateMoistureMap(PerlinNoise& noise) {
    double scale = config.forestScale * 0.7;
    for (int x = 0; x < mapWidth; x++) {
        for (int y = 0; y < mapHeight; y++) {
            double nx = x * scale;
            double ny = y * scale;
            double m = noise.FBM(nx, ny, 4, 2.0, 0.5);
            moistureMap[x][y] = static_cast<float>((m + 1.0) * 0.5);
        }
    }
}

// ============================================================
// Forest Map
// ============================================================

void MapGenerator::GenerateForestMap(PerlinNoise& noise) {
    for (int x = 0; x < mapWidth; x++) {
        for (int y = 0; y < mapHeight; y++) {
            double nx = x * config.forestScale;
            double ny = y * config.forestScale;

            double f = noise.FBM(nx, ny, 5, 2.0, 0.55);
            f = (f + 1.0) * 0.5;

            // Forest grows better in moist areas
            float moisture = moistureMap[x][y];
            f = f * (0.5 + 0.5 * moisture);

            forestMap[x][y] = static_cast<float>(f);
        }
    }
}

// ============================================================
// Lake Generation
// ============================================================

void MapGenerator::GenerateLakes(PerlinNoise& noise) {
    std::uniform_real_distribution<double> radiusDist(config.lakeMinRadius,
                                                       config.lakeMaxRadius);
    int margin = static_cast<int>(config.lakeMaxRadius) + 10;

    for (int i = 0; i < config.numLakes; i++) {
        Point2D lakePos = FindLakePosition(noise);
        if (lakePos.x < 0) {
            // Fallback: random position
            std::uniform_int_distribution<int> xDist(margin,
                                                      mapWidth - margin);
            std::uniform_int_distribution<int> yDist(margin,
                                                      mapHeight - margin);
            lakePos = {xDist(rng), yDist(rng)};
        }

        double radius = radiusDist(rng);
        double irregularity = 0.3 + 0.4 * (rng() % 100) / 100.0;
        FloodFillWater(lakePos.x, lakePos.y, static_cast<float>(radius),
                        static_cast<float>(irregularity), noise);
    }
}

Point2D MapGenerator::FindLakePosition(PerlinNoise& noise) const {
    // Lakes form in low terrain areas with high moisture
    int margin = static_cast<int>(config.lakeMaxRadius) + 20;
    Point2D best = {-1, -1};
    float bestScore = -1.0f;

    // Sample random positions and pick the best one
    std::mt19937 localRng(seed + 100);
    std::uniform_int_distribution<int> xDist(margin, mapWidth - margin);
    std::uniform_int_distribution<int> yDist(margin, mapHeight - margin);

    for (int attempt = 0; attempt < 50; attempt++) {
        int x = xDist(localRng);
        int y = yDist(localRng);

        // Score: low height + high moisture = good lake spot
        float score = (1.0f - heightMap[x][y]) * 0.6f +
                      moistureMap[x][y] * 0.4f;

        // Penalize if too close to another lake
        bool tooClose = false;
        int checkRadius = static_cast<int>(config.lakeMaxRadius * 3);
        for (int dx = -checkRadius; dx <= checkRadius && !tooClose; dx += 5) {
            for (int dy = -checkRadius; dy <= checkRadius && !tooClose;
                 dy += 5) {
                int cx = x + dx;
                int cy = y + dy;
                if (IsInBounds(cx, cy) && waterMask[cx][cy]) {
                    tooClose = true;
                }
            }
        }

        if (!tooClose && score > bestScore) {
            bestScore = score;
            best = {x, y};
        }
    }

    return best;
}

void MapGenerator::FloodFillWater(int cx, int cy, float radius,
                                    float irregularity, PerlinNoise& noise) {
    int r = static_cast<int>(radius) + 5;
    for (int dx = -r; dx <= r; dx++) {
        for (int dy = -r; dy <= r; dy++) {
            int x = cx + dx;
            int y = cy + dy;
            if (!IsInBounds(x, y)) continue;

            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));

            // Use noise to create irregular shoreline
            double angle = std::atan2(static_cast<double>(dy),
                                       static_cast<double>(dx));
            double noiseVal = noise.Noise(angle * 3.0 + cx * 0.01,
                                           dist * 0.1 + cy * 0.01);
            float adjustedRadius = radius *
                (1.0f + static_cast<float>(irregularity * noiseVal));

            if (dist < adjustedRadius) {
                waterMask[x][y] = true;
            }
        }
    }
}

// ============================================================
// River Generation
// ============================================================

void MapGenerator::GenerateRivers() {
    for (int i = 0; i < config.numRivers; i++) {
        Point2D source = FindRiverSource();
        if (source.x < 0) continue;

        // Pick a general direction (0=N, 1=E, 2=S, 3=W, or towards a lake)
        int direction = rng() % 4;
        TraceRiver(source, direction);
    }
}

Point2D MapGenerator::FindRiverSource() const {
    // Rivers start from higher terrain areas
    int margin = 20;
    Point2D best = {-1, -1};
    float bestHeight = 0.0f;

    std::mt19937 localRng(seed + 200 + config.numRivers);
    std::uniform_int_distribution<int> xDist(margin, mapWidth - margin);
    std::uniform_int_distribution<int> yDist(margin, mapHeight - margin);

    for (int attempt = 0; attempt < 30; attempt++) {
        int x = xDist(localRng);
        int y = yDist(localRng);

        if (waterMask[x][y]) continue;

        if (heightMap[x][y] > bestHeight) {
            bestHeight = heightMap[x][y];
            best = {x, y};
        }
    }

    return best;
}

void MapGenerator::TraceRiver(Point2D start, int direction) {
    double riverW = config.riverWidth;
    Point2D current = start;

    // Direction vectors with some bias
    // 0=North(y-), 1=East(x+), 2=South(y+), 3=West(x-)
    int dx_bias[] = {0, 1, 0, -1};
    int dy_bias[] = {-1, 0, 1, 0};

    std::uniform_real_distribution<double> wobble(-2.0, 2.0);
    std::uniform_int_distribution<int> turnChance(0, 100);

    int maxSteps = mapWidth + mapHeight;

    for (int step = 0; step < maxSteps; step++) {
        // Carve river at current position
        int halfW = static_cast<int>(riverW);
        for (int wx = -halfW; wx <= halfW; wx++) {
            for (int wy = -halfW; wy <= halfW; wy++) {
                int rx = current.x + wx;
                int ry = current.y + wy;
                if (IsInBounds(rx, ry)) {
                    float dist = std::sqrt(static_cast<float>(
                        wx * wx + wy * wy));
                    if (dist <= static_cast<float>(riverW)) {
                        waterMask[rx][ry] = true;
                    }
                }
            }
        }

        // Move in general direction with wobble
        double moveX = dx_bias[direction] + wobble(rng) * 0.5;
        double moveY = dy_bias[direction] + wobble(rng) * 0.5;

        // Follow terrain gradient slightly (rivers flow downhill)
        if (IsInBounds(current.x + 1, current.y) &&
            IsInBounds(current.x - 1, current.y)) {
            double gradX = heightMap[current.x + 1][current.y] -
                           heightMap[current.x - 1][current.y];
            moveX -= gradX * 3.0; // Move towards lower terrain
        }
        if (IsInBounds(current.x, current.y + 1) &&
            IsInBounds(current.x, current.y - 1)) {
            double gradY = heightMap[current.x][current.y + 1] -
                           heightMap[current.x][current.y - 1];
            moveY -= gradY * 3.0;
        }

        current.x += static_cast<int>(std::round(moveX));
        current.y += static_cast<int>(std::round(moveY));

        // Occasionally change direction slightly
        if (turnChance(rng) < 5) {
            direction = (direction + (rng() % 2 == 0 ? 1 : 3)) % 4;
        }

        // Stop if we hit the edge or an existing water body
        if (!IsInBounds(current.x, current.y)) break;

        // Check if we've reached a lake (stop the river there)
        bool hitExistingWater = false;
        for (int checkR = -2; checkR <= 2; checkR++) {
            for (int checkC = -2; checkC <= 2; checkC++) {
                int cx = current.x + checkR;
                int cy = current.y + checkC;
                if (IsInBounds(cx, cy) && waterMask[cx][cy]) {
                    // Check if it's part of a lake (larger water body)
                    int waterCount = 0;
                    for (int lx = -5; lx <= 5; lx++) {
                        for (int ly = -5; ly <= 5; ly++) {
                            if (IsInBounds(cx + lx, cy + ly) &&
                                waterMask[cx + lx][cy + ly]) {
                                waterCount++;
                            }
                        }
                    }
                    if (waterCount > 30) {
                        hitExistingWater = true;
                    }
                }
            }
        }
        if (hitExistingWater) break;
    }
}

void MapGenerator::SmoothWaterEdges() {
    // Simple smoothing pass: fill in single-tile gaps in water
    auto tempWater = waterMask;

    for (int x = 1; x < mapWidth - 1; x++) {
        for (int y = 1; y < mapHeight - 1; y++) {
            if (!waterMask[x][y]) {
                int waterNeighbors = 0;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dx == 0 && dy == 0) continue;
                        if (waterMask[x + dx][y + dy]) waterNeighbors++;
                    }
                }
                // Fill in if mostly surrounded by water
                if (waterNeighbors >= 6) {
                    tempWater[x][y] = true;
                }
            } else {
                // Remove isolated water pixels
                int waterNeighbors = 0;
                for (int dx = -1; dx <= 1; dx++) {
                    for (int dy = -1; dy <= 1; dy++) {
                        if (dx == 0 && dy == 0) continue;
                        if (waterMask[x + dx][y + dy]) waterNeighbors++;
                    }
                }
                if (waterNeighbors <= 1) {
                    tempWater[x][y] = false;
                }
            }
        }
    }

    waterMask = tempWater;
}

// ============================================================
// Starting Zone Placement
// ============================================================

std::vector<StartingZone> MapGenerator::PlaceStartingZones(int numPlayers) {
    std::vector<StartingZone> zones;

    // Strategy: place players roughly evenly around the map
    // Use a circle pattern for fairness
    double centerX = mapWidth *32.0/ 2.0;
    double centerY = mapHeight *32.0/ 2.0;
    double placementRadius = std::min(mapWidth, mapHeight) * 32.0 * 0.35;

    for (int i = 0; i < numPlayers; i++) {
        double angle = (2.0 * M_PI * i) / numPlayers +
                       (M_PI / 4.0); // Offset so not axis-aligned

        // Ideal position
        int idealX = static_cast<int>(centerX + placementRadius *
                                       std::cos(angle));
        int idealY = static_cast<int>(centerY + placementRadius *
                                       std::sin(angle));

        // Search nearby for a valid position
        StartingZone zone;
        zone.playerId = i;

        bool found = false;
        int searchRadius = config.startingZoneRadius * 3;

        // Spiral search from ideal position
        for (int r = 0; r < searchRadius && !found; r += 3) {
            for (int a = 0; a < 36 && !found; a++) {
                double searchAngle = (2.0 * M_PI * a) / 36.0;
                int px = idealX + static_cast<int>(r * std::cos(searchAngle));
                int py = idealY + static_cast<int>(r * std::sin(searchAngle));

                Point2D pos = {px, py};
                if (IsValidStartPosition(pos, zones)) {
                    zone.center = pos;
                    found = true;
                }
            }
        }

        if (!found) {
            // Emergency fallback: random position
            std::uniform_int_distribution<int> xDist(
                config.startingZoneRadius + 10,
                mapWidth - config.startingZoneRadius - 10);
            std::uniform_int_distribution<int> yDist(
                config.startingZoneRadius + 10,
                mapHeight - config.startingZoneRadius - 10);

            for (int attempt = 0; attempt < 1000; attempt++) {
                Point2D pos = {xDist(rng), yDist(rng)};
                if (IsValidStartPosition(pos, zones)) {
                    zone.center = pos;
                    found = true;
                    break;
                }
            }

            if (!found) {
                // Absolute fallback
                zone.center = {idealX, idealY};
                // Clamp to bounds
                zone.center.x = std::clamp(zone.center.x,
                    config.startingZoneRadius + 5,
                    mapWidth - config.startingZoneRadius - 5);
                zone.center.y = std::clamp(zone.center.y,
                    config.startingZoneRadius + 5,
                    mapHeight - config.startingZoneRadius - 5);
            }
        }

        // Set hut and peasant positions relative to center
        zone.hutPosition = {zone.center.x, zone.center.y};
        zone.peasantPosition = {zone.center.x + 2, zone.center.y + 2};

        // Clear the starting area
        ClearStartingArea(zone);

        zones.push_back(zone);
        std::cout << "[MapGen] Player " << i << " start at ("
                  << zone.center.x << ", " << zone.center.y << ")"
                  << std::endl;
    }

    return zones;
}

bool MapGenerator::IsValidStartPosition(const Point2D& pos,
    const std::vector<StartingZone>& existing) const {

    int margin = config.startingZoneRadius + 5;

    // Check bounds
    if (pos.x < margin || pos.x >= mapWidth - margin ||
        pos.y < margin || pos.y >= mapHeight - margin) {
        return false;
    }

    // Check not on water (center area should be dry)
    int checkR = config.startingZoneRadius / 2;
    for (int dx = -checkR; dx <= checkR; dx += 3) {
        for (int dy = -checkR; dy <= checkR; dy += 3) {
            int cx = pos.x + dx;
            int cy = pos.y + dy;
            if (IsInBounds(cx, cy) && waterMask[cx][cy]) {
                return false;
            }
        }
    }

    // Check minimum distance from other starts
    for (const auto& zone : existing) {
        float dist = DistanceBetween(pos, zone.center);
        if (dist < config.minPlayerDistance) {
            return false;
        }
    }

    return true;
}

void MapGenerator::ClearStartingArea(StartingZone& zone) {
    int r = config.startingZoneRadius;
    for (int dx = -r; dx <= r; dx++) {
        for (int dy = -r; dy <= r; dy++) {
            int x = zone.center.x + dx;
            int y = zone.center.y + dy;
            if (!IsInBounds(x, y)) continue;

            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (dist <= r) {
                waterMask[x][y] = false;
                treeMask[x][y] = false;
                stoneMask[x][y] = false;

                // Mark inner area as path (ensures walkability)
                if (dist <= r * 0.6f) {
                    pathMask[x][y] = true;
                }
            }
        }
    }
}

void MapGenerator::EnsureWaterAccess(StartingZone& zone) {
    // Find nearest water
    Point2D water = FindNearestWater(zone.center, config.waterAccessMaxDist);

    if (water.x < 0) {
        // No water nearby - create a small pond
        int pondDist = config.startingZoneRadius + 10;
        std::uniform_real_distribution<double> angleDist(0, 2.0 * M_PI);
        double angle = angleDist(rng);

        int pondX = zone.center.x +
                    static_cast<int>(pondDist * std::cos(angle));
        int pondY = zone.center.y +
                    static_cast<int>(pondDist * std::sin(angle));

        pondX = std::clamp(pondX, 10, mapWidth - 10);
        pondY = std::clamp(pondY, 10, mapHeight - 10);

        PerlinNoise pondNoise(seed + zone.playerId * 1000 + 500);
        FloodFillWater(pondX, pondY, 5.0f, 0.2f, pondNoise);

        water = {pondX, pondY};
        std::cout << "[MapGen] Created pond for player " << zone.playerId
                  << " at (" << pondX << ", " << pondY << ")" << std::endl;
    }

    zone.nearestWaterAccess = water;

    // Carve a clear path from start to water
    CarvePathBetween(zone.center, water, config.pathWidth);
}

void MapGenerator::PlaceStartingResources(StartingZone& zone) {
    int r = config.startingResourceRadius;

    // Place guaranteed trees in a ring around the starting area
    int treeRingMin = config.startingZoneRadius - 5;
    int treeRingMax = config.startingResourceRadius;

    std::uniform_real_distribution<double> angleDist(0, 2.0 * M_PI);
    std::uniform_int_distribution<int> distDist(treeRingMin, treeRingMax);

    int treesPlaced = 0;
    int maxAttempts = config.minTreesPerStart * 10;

    for (int attempt = 0;
         attempt < maxAttempts && treesPlaced < config.minTreesPerStart;
         attempt++) {
        double angle = angleDist(rng);
        int dist = distDist(rng);

        int tx = zone.center.x + static_cast<int>(dist * std::cos(angle));
        int ty = zone.center.y + static_cast<int>(dist * std::sin(angle));

        if (IsInBounds(tx, ty) && !waterMask[tx][ty] && !pathMask[tx][ty] &&
            !stoneMask[tx][ty]) {
            treeMask[tx][ty] = true;
            zone.guaranteedTreePositions.push_back({tx, ty});
            treesPlaced++;
        }
    }

    // Place guaranteed stones
    int stonesPlaced = 0;
    maxAttempts = config.minStonesPerStart * 10;

    for (int attempt = 0;
         attempt < maxAttempts && stonesPlaced < config.minStonesPerStart;
         attempt++) {
        double angle = angleDist(rng);
        int dist = distDist(rng);

        int sx = zone.center.x + static_cast<int>(dist * std::cos(angle));
        int sy = zone.center.y + static_cast<int>(dist * std::sin(angle));

        if (IsInBounds(sx, sy) && !waterMask[sx][sy] && !pathMask[sx][sy] &&
            !treeMask[sx][sy]) {
            stoneMask[sx][sy] = true;
            zone.guaranteedStonePositions.push_back({sx, sy});
            stonesPlaced++;
        }
    }

    std::cout << "[MapGen] Player " << zone.playerId << ": "
              << treesPlaced << " trees, " << stonesPlaced << " stones"
              << std::endl;
}

// ============================================================
// Path Generation
// ============================================================

void MapGenerator::GenerateMainPaths(const std::vector<StartingZone>& zones) {
    // Connect all starting zones to each other
    for (size_t i = 0; i < zones.size(); i++) {
        for (size_t j = i + 1; j < zones.size(); j++) {
            CarvePathBetween(zones[i].center, zones[j].center,
                             config.pathWidth);
        }
    }

    // Add some extra random paths through the map for exploration
    std::uniform_int_distribution<int> xDist(50, mapWidth - 50);
    std::uniform_int_distribution<int> yDist(50, mapHeight - 50);

    for (int i = 0; i < config.numMainPaths; i++) {
        Point2D from = {xDist(rng), yDist(rng)};
        Point2D to = {xDist(rng), yDist(rng)};

        // Skip if either endpoint is water
        if (IsInBounds(from.x, from.y) && IsInBounds(to.x, to.y) &&
            !waterMask[from.x][from.y] && !waterMask[to.x][to.y]) {

            float dist = DistanceBetween(from, to);
            if (dist > mapWidth * 0.2f) { // Only long paths
                CarvePathBetween(from, to, config.pathWidth * 0.8);
            }
        }
    }
}

void MapGenerator::CarvePathBetween(const Point2D& from, const Point2D& to,
                                     double width) {
    // Use A* to find a path that avoids water, then widen it
    auto pathTiles = FindPathAStar(from, to, false);

    if (pathTiles.empty()) {
        // If no path found (e.g., water blocking), try allowing water crossings
        // with bridges (just carve through)
        pathTiles = FindPathAStar(from, to, true);
    }

    if (pathTiles.empty()) {
        // Fallback: straight line
        int steps = static_cast<int>(DistanceBetween(from, to));
        for (int s = 0; s <= steps; s++) {
            float t = static_cast<float>(s) / std::max(steps, 1);
            int x = static_cast<int>(from.x + t * (to.x - from.x));
            int y = static_cast<int>(from.y + t * (to.y - from.y));

            int halfW = static_cast<int>(width);
            for (int dx = -halfW; dx <= halfW; dx++) {
                for (int dy = -halfW; dy <= halfW; dy++) {
                    int px = x + dx;
                    int py = y + dy;
                    if (IsInBounds(px, py)) {
                        float dist = std::sqrt(static_cast<float>(
                            dx * dx + dy * dy));
                        if (dist <= width) {
                            pathMask[px][py] = true;
                            treeMask[px][py] = false;
                            stoneMask[px][py] = false;
                            // Don't remove water for straight lines
                        }
                    }
                }
            }
        }
        return;
    }

    // Carve the path with given width
    for (const auto& tile : pathTiles) {
        int halfW = static_cast<int>(width);
        for (int dx = -halfW; dx <= halfW; dx++) {
            for (int dy = -halfW; dy <= halfW; dy++) {
                int px = tile.x + dx;
                int py = tile.y + dy;
                if (IsInBounds(px, py)) {
                    float dist = std::sqrt(static_cast<float>(
                        dx * dx + dy * dy));
                    if (dist <= width) {
                        pathMask[px][py] = true;
                        treeMask[px][py] = false;
                        stoneMask[px][py] = false;
                        // Don't remove water - paths go around water
                    }
                }
            }
        }
    }
}

std::vector<Point2D> MapGenerator::FindPathAStar(const Point2D& from,
    const Point2D& to, bool allowWater) const {

    // Simplified A* on the generation grid
    // Use step size to make it faster on large maps
    int step = std::max(1, std::min(mapWidth, mapHeight) / 500);

    struct Node {
        Point2D pos;
        float gCost;
        float fCost;

        bool operator>(const Node& other) const {
            return fCost > other.fCost;
        }
    };

    auto heuristic = [&](const Point2D& a, const Point2D& b) -> float {
        return std::abs(a.x - b.x) + std::abs(a.y - b.y);
    };

    // Snap to step grid
    Point2D start = {(from.x / step) * step, (from.y / step) * step};
    Point2D end = {(to.x / step) * step, (to.y / step) * step};

    std::priority_queue<Node, std::vector<Node>, std::greater<Node>> openSet;
    std::unordered_map<int64_t, float> gScore;
    std::unordered_map<int64_t, Point2D> cameFrom;

    auto key = [&](const Point2D& p) -> int64_t {
        return static_cast<int64_t>(p.x) * mapHeight + p.y;
    };

    openSet.push({start, 0.0f, heuristic(start, end)});
    gScore[key(start)] = 0.0f;

    int dx[] = {step, -step, 0, 0, step, step, -step, -step};
    int dy[] = {0, 0, step, -step, step, -step, step, -step};
    float costs[] = {1.0f, 1.0f, 1.0f, 1.0f, 1.414f, 1.414f, 1.414f, 1.414f};

    int maxIterations = (mapWidth * mapHeight) / (step * step);
    int iterations = 0;

    while (!openSet.empty() && iterations < maxIterations) {
        iterations++;
        Node current = openSet.top();
        openSet.pop();

        if (std::abs(current.pos.x - end.x) <= step &&
            std::abs(current.pos.y - end.y) <= step) {
            // Reconstruct path
            std::vector<Point2D> path;
            Point2D cur = current.pos;
            while (!(cur.x == start.x && cur.y == start.y)) {
                path.push_back(cur);
                auto it = cameFrom.find(key(cur));
                if (it == cameFrom.end()) break;
                cur = it->second;
            }
            path.push_back(start);
            std::reverse(path.begin(), path.end());
            return path;
        }

        for (int d = 0; d < 8; d++) {
            Point2D neighbor = {current.pos.x + dx[d],
                                current.pos.y + dy[d]};

            if (!IsInBounds(neighbor.x, neighbor.y)) continue;

            float moveCost = costs[d] * step;

            // Add penalty for water if not allowed
            if (waterMask[neighbor.x][neighbor.y]) {
                if (!allowWater) {
                    moveCost *= 100.0f; // Heavy penalty, not infinite
                } else {
                    moveCost *= 5.0f;
                }
            }

            // Slight preference for existing paths
            if (pathMask[neighbor.x][neighbor.y]) {
                moveCost *= 0.5f;
            }

            // Slight penalty for dense forest (paths look more natural
            // when they curve around dense areas slightly)
            if (forestMap[neighbor.x][neighbor.y] > 0.7f) {
                moveCost *= 1.3f;
            }

            float tentativeG = gScore[key(current.pos)] + moveCost;
            auto it = gScore.find(key(neighbor));

            if (it == gScore.end() || tentativeG < it->second) {
                gScore[key(neighbor)] = tentativeG;
                cameFrom[key(neighbor)] = current.pos;
                float f = tentativeG + heuristic(neighbor, end);
                openSet.push({neighbor, tentativeG, f});
            }
        }
    }

    return {}; // No path found
}

// ============================================================
// Tree and Stone Placement
// ============================================================

void MapGenerator::PlaceTrees() {
    std::uniform_real_distribution<double> prob(0.0, 1.0);

    for (int x = 0; x < mapWidth; x++) {
        for (int y = 0; y < mapHeight; y++) {
            if (waterMask[x][y]) continue;
            if (pathMask[x][y]) continue;
            if (treeMask[x][y]) continue; // Already placed (starting resources)

            float forestVal = forestMap[x][y];

            // Only place trees in forested areas
            if (forestVal < (1.0f - config.forestDensity)) continue;

            // Probability based on forest density
            double treeProbability = config.treeDensity *
                (forestVal - (1.0f - config.forestDensity)) /
                config.forestDensity;

            if (prob(rng) < treeProbability) {
                treeMask[x][y] = true;
            }
        }
    }
}

void MapGenerator::PlaceStones(PerlinNoise& noise) {
    std::uniform_real_distribution<double> prob(0.0, 1.0);
    double stoneScale = config.forestScale * 1.5;

    for (int x = 0; x < mapWidth; x++) {
        for (int y = 0; y < mapHeight; y++) {
            if (waterMask[x][y]) continue;
            if (pathMask[x][y]) continue;
            if (treeMask[x][y]) continue;
            if (stoneMask[x][y]) continue;

            // Stones appear in clusters based on noise
            double stoneNoise = noise.FBM(x * stoneScale, y * stoneScale,
                                           3, 2.0, 0.5);
            stoneNoise = (stoneNoise + 1.0) * 0.5;

            // Also more common in hilly areas (high terrain)
            double heightFactor = heightMap[x][y];

            // Very sparse placement
            double stoneProbability = 0.002 * stoneNoise * stoneNoise *
                                     (0.5 + heightFactor);

            if (prob(rng) < stoneProbability) {
                stoneMask[x][y] = true;
            }
        }
    }
}

// ============================================================
// Connectivity Verification
// ============================================================

bool MapGenerator::VerifyAllStartsConnected(
    const std::vector<StartingZone>& zones) const {
    if (zones.size() <= 1) return true;

    for (size_t i = 1; i < zones.size(); i++) {
        if (!BFSPathExists(zones[0].center, zones[i].center)) {
            std::cout << "[MapGen] Player 0 cannot reach player " << i
                      << std::endl;
            return false;
        }
    }
    return true;
}

bool MapGenerator::BFSPathExists(const Point2D& from,
                                  const Point2D& to) const {
    int step = std::max(2, std::min(mapWidth, mapHeight) / 200);

    std::queue<Point2D> queue;
    std::unordered_set<int64_t> visited;

    auto key = [&](const Point2D& p) -> int64_t {
        return static_cast<int64_t>(p.x / step) * (mapHeight / step + 1) +
               p.y / step;
    };

    queue.push(from);
    visited.insert(key(from));

    int dx[] = {step, -step, 0, 0};
    int dy[] = {0, 0, step, -step};

    while (!queue.empty()) {
        Point2D current = queue.front();
        queue.pop();

        if (std::abs(current.x - to.x) <= step * 2 &&
            std::abs(current.y - to.y) <= step * 2) {
            return true;
        }

        for (int d = 0; d < 4; d++) {
            Point2D next = {current.x + dx[d], current.y + dy[d]};

            if (!IsInBounds(next.x, next.y)) continue;
            if (waterMask[next.x][next.y]) continue;

            int64_t k = key(next);
            if (visited.count(k)) continue;
            visited.insert(k);

            queue.push(next);
        }
    }

    return false;
}

// ============================================================
// Apply to Map
// ============================================================

void MapGenerator::ApplyToMap(Map* map,
                               const std::vector<StartingZone>& zones) {
    // Set tile types
    for (int x = 0; x < mapWidth; x++) {
        for (int y = 0; y < mapHeight; y++) {
            Tile* tile = map->GetTile(x, y);
            if (!tile) continue;

            if (waterMask[x][y]) {
                tile->SetType(TileType::WATER);
                tile->SetWalkable(false);
            } else if (pathMask[x][y]) {
                tile->SetType(TileType::DIRT);
                tile->SetWalkable(true);
            } else {
                tile->SetType(TileType::GRASS);
                tile->SetWalkable(true);
            }
        }
    }

    // Place tree obstacles
    for (int x = 0; x < mapWidth; x++) {
        for (int y = 0; y < mapHeight; y++) {
            if (treeMask[x][y]) {
                auto tree = std::make_shared<Obstacle>(
                    map->GetNextEntityId(), ObstacleType::TREE, static_cast<float>(x), static_cast<float>(y));
                map->AddObstacle(tree);
            }
        }
    }

    // Place stone obstacles
    for (int x = 0; x < mapWidth; x++) {
        for (int y = 0; y < mapHeight; y++) {
            if (stoneMask[x][y]) {
                auto stone = std::make_shared<Obstacle>(
                    map->GetNextEntityId(), ObstacleType::ROCK, static_cast<float>(x), static_cast<float>(y));
                map->AddObstacle(stone);
            }
        }
    }

    // Place starting buildings and units for each player
    for (const auto& zone : zones) {
        // Place hut
        auto hut = std::make_shared<Building>(
            map->GetNextEntityId(), zone.playerId, BuildingType::HUT);
        hut->SetPosition(static_cast<float>(zone.hutPosition.x*32.0f + 16.0f),
                         static_cast<float>(zone.hutPosition.y*32.0f + 16.0f));
        hut->StartConstruction();
        hut->SetConstructionProgress(1.0f); // Already built
        map->AddEntity(hut);

        // Place peasant
        auto peasant = std::make_shared<Peasant>(
            map->GetNextEntityId(), zone.playerId);
        peasant->SetPosition(
            static_cast<float>(zone.peasantPosition.x*32.0f + 16.0f),
            static_cast<float>(zone.peasantPosition.y*32.0f + 16.0f));
        peasant->SetMap(map);
        map->AddEntity(peasant);
    }
}

// ============================================================
// Utility Functions
// ============================================================

float MapGenerator::DistanceBetween(const Point2D& a,
                                     const Point2D& b) const {
    float dx = static_cast<float>(a.x - b.x);
    float dy = static_cast<float>(a.y - b.y);
    return std::sqrt(dx * dx + dy * dy);
}

bool MapGenerator::IsInBounds(int x, int y) const {
    return x >= 0 && x < mapWidth && y >= 0 && y < mapHeight;
}

std::vector<Point2D> MapGenerator::GetTilesInRadius(const Point2D& center,
                                                     int radius) const {
    std::vector<Point2D> tiles;
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            int x = center.x + dx;
            int y = center.y + dy;
            if (!IsInBounds(x, y)) continue;
            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (dist <= radius) {
                tiles.push_back({x, y});
            }
        }
    }
    return tiles;
}

std::vector<Point2D> MapGenerator::GetLandTilesInRadius(
    const Point2D& center, int radius) const {
    std::vector<Point2D> tiles;
    for (int dx = -radius; dx <= radius; dx++) {
        for (int dy = -radius; dy <= radius; dy++) {
            int x = center.x + dx;
            int y = center.y + dy;
            if (!IsInBounds(x, y)) continue;
            if (waterMask[x][y]) continue;
            float dist = std::sqrt(static_cast<float>(dx * dx + dy * dy));
            if (dist <= radius) {
                tiles.push_back({x, y});
            }
        }
    }
    return tiles;
}

Point2D MapGenerator::FindNearestWater(const Point2D& from,
                                        int maxSearch) const {
    // BFS outward to find water
    std::queue<Point2D> queue;
    std::unordered_set<int64_t> visited;

    auto key = [&](const Point2D& p) -> int64_t {
        return static_cast<int64_t>(p.x) * mapHeight + p.y;
    };

    queue.push(from);
    visited.insert(key(from));

    int dx[] = {1, -1, 0, 0, 1, 1, -1, -1};
    int dy[] = {0, 0, 1, -1, 1, -1, 1, -1};

    while (!queue.empty()) {
        Point2D current = queue.front();
        queue.pop();

        float dist = DistanceBetween(from, current);
        if (dist > maxSearch) continue;

        if (waterMask[current.x][current.y]) {
            return current;
        }

        for (int d = 0; d < 8; d++) {
            Point2D next = {current.x + dx[d], current.y + dy[d]};
            if (!IsInBounds(next.x, next.y)) continue;

            int64_t k = key(next);
            if (visited.count(k)) continue;
            visited.insert(k);

            queue.push(next);
        }
    }

    return {-1, -1}; // Not found
}

void MapGenerator::DigChannelToWater(const Point2D& from,
                                      const Point2D& waterTile,
                                      double width) {
    auto path = FindPathAStar(from, waterTile, true);
    if (path.empty()) return;

    for (const auto& tile : path) {
        int halfW = static_cast<int>(width);
        for (int dx = -halfW; dx <= halfW; dx++) {
            for (int dy = -halfW; dy <= halfW; dy++) {
                int px = tile.x + dx;
                int py = tile.y + dy;
                if (IsInBounds(px, py)) {
                    float dist = std::sqrt(static_cast<float>(
                        dx * dx + dy * dy));
                    if (dist <= width) {
                        pathMask[px][py] = true;
                        treeMask[px][py] = false;
                        stoneMask[px][py] = false;
                    }
                }
            }
        }
    }
}