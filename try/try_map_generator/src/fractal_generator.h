#pragma once

#include "map_data.h"
#include <random>
#include <vector>
#include <utility>

struct WaterParams {
    float densityMultiplier = 1.0f;   // multiplier on number of river sources
    float widthMultiplier = 1.0f;     // multiplier on river width
    int extraSources = 0;             // additional river sources beyond base count
};

struct MetalParams {
    float densityMultiplier = 1.0f;   // multiplier on number of vein clusters
    float widthMultiplier = 1.0f;     // multiplier on vein width
    float intensityMultiplier = 1.0f; // multiplier on vein intensity/richness
    int extraVeins = 0;               // additional vein clusters beyond base count
};

class FractalGenerator {
public:
    FractalGenerator(std::mt19937& rng);

    // Generate fractal river system (water)
    void generateRiverSystem(MapData& map, int numBaseSources, const WaterParams& params);

    // Generate fractal metal veins at a location
    void generateMetalVeins(MapData& map, float centerX, float centerY,
                            float radius, float intensity, bool isTangle,
                            const MetalParams& params);

    // Generate metal veins for common area
    void generateCommonMetalVeins(MapData& map, const std::vector<StartingArea>& startAreas,
                                   const MetalParams& params);

    // Ensure water doesn't divide the map
    void validateWaterConnectivity(MapData& map);

private:
    std::mt19937& rng_;

    void growRiverBranch(MapData& map, float x, float y, float angle,
                         float width, float widthMul, int depth, int maxDepth,
                         float branchChance,
                         std::vector<std::pair<int,int>>& waterTiles);

    void growMetalVein(MapData& map, float x, float y, float angle,
                       float width, float intensity, float widthMul,
                       int depth, int maxDepth);

    void carveRiverPoint(MapData& map, int cx, int cy, float width,
                         std::vector<std::pair<int,int>>& waterTiles);

    void carveMetalPoint(MapData& map, int cx, int cy, float width, float intensity);

    bool isNearStartArea(MapData& map, int x, int y, int margin);
};