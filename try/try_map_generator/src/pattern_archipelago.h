#pragma once

#include "map_data.h"
#include "fractal_generator.h"
#include "noise.h"
#include <random>

// Bonus pattern: "Archipelago" - players start on connected landmasses
// with narrow land bridges, surrounded by scattered resources
class PatternArchipelago {
public:
    PatternArchipelago(std::mt19937& rng, PerlinNoise& noise);
    void generate(MapData& map, int numPlayers);

private:
    std::mt19937& rng_;
    PerlinNoise& noise_;
    FractalGenerator fractal_;

    void generateTerrain(MapData& map);
    void placeStartingAreas(MapData& map, int numPlayers);
    void ensureConnectivity(MapData& map);
    void fillVegetation(MapData& map);
    void placeMetalDeposits(MapData& map);
};