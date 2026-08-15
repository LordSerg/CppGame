#pragma once

#include "map_data.h"
#include <random>
#include <vector>
#include <utility>

struct FractalBranch {
    float startX, startY;
    float angle;
    float length;
    float width;
    int depth;
};

class FractalGenerator {
public:
    FractalGenerator(std::mt19937& rng);

    // Generate fractal river system (water)
    void generateRiverSystem(MapData& map, int numSources);

    // Generate fractal metal veins
    void generateMetalVeins(MapData& map, float centerX, float centerY,
                            float radius, float intensity, bool isTangle = false);

    // Generate metal veins for common area
    void generateCommonMetalVeins(MapData& map, const std::vector<StartingArea>& startAreas);

    // Ensure water doesn't divide the map - validation and fixup
    void validateWaterConnectivity(MapData& map);

private:
    std::mt19937& rng_;

    void growRiverBranch(MapData& map, float x, float y, float angle,
                         float width, int depth, int maxDepth,
                         std::vector<std::pair<int,int>>& waterTiles);

    void growMetalVein(MapData& map, float x, float y, float angle,
                       float width, float intensity, int depth, int maxDepth);

    void carveRiverPoint(MapData& map, int cx, int cy, float width,
                         std::vector<std::pair<int,int>>& waterTiles);

    void carveMetalPoint(MapData& map, int cx, int cy, float width, float intensity);

    bool isNearStartArea(MapData& map, int x, int y, int margin);
};