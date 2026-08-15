#pragma once

#include "map_data.h"
#include <random>
#include <vector>
#include <utility>

struct WaterParams {
    float densityMultiplier = 1.0f;
    float widthMultiplier = 1.0f;
    int extraSources = 0;
};

struct MetalParams {
    float densityMultiplier = 1.0f;
    float widthMultiplier = 1.0f;
    float intensityMultiplier = 1.0f;
    int extraVeins = 0;
};

class FractalGenerator {
public:
    FractalGenerator(std::mt19937& rng);

    void generateRiverSystem(MapData& map, int numBaseSources, const WaterParams& params);

    void generateMetalVeins(MapData& map, float centerX, float centerY,
                            float radius, float intensity, bool isTangle,
                            const MetalParams& params);

    void generateCommonMetalVeins(MapData& map, const std::vector<StartingArea>& startAreas,
                                   const MetalParams& params);

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