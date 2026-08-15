#pragma once

#include "map_data.h"
#include "fractal_generator.h"
#include "noise.h"
#include <random>

class PatternArchipelago {
public:
    PatternArchipelago(std::mt19937& rng, PerlinNoise& noise);
    void generate(MapData& map, int numPlayers,
                  const WaterParams& waterParams, const MetalParams& metalParams);

private:
    std::mt19937& rng_;
    PerlinNoise& noise_;
    FractalGenerator fractal_;

    void generateTerrain(MapData& map, const WaterParams& waterParams);
    void placeStartingAreas(MapData& map, int numPlayers);
    void ensureConnectivity(MapData& map);
    void fillVegetation(MapData& map);
    void placeMetalDeposits(MapData& map, const MetalParams& params);
};