#pragma once

#include "map_data.h"
#include "fractal_generator.h"
#include "placement.h"
#include "noise.h"
#include <random>

class PatternStar {
public:
    PatternStar(std::mt19937& rng, PerlinNoise& noise);
    void generate(MapData& map, int numPlayers, PlacementMode placement,
                  const WaterParams& waterParams, const MetalParams& metalParams);

private:
    std::mt19937& rng_;
    PerlinNoise& noise_;
    FractalGenerator fractal_;
    PlacementStrategy placement_;

    void carvePaths(MapData& map);
    void fillForest(MapData& map);
    void placeMetalDeposits(MapData& map, const MetalParams& params);

    void carvePath(MapData& map, int x1, int y1, int x2, int y2, int width);
};