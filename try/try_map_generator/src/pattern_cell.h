#pragma once

#include "map_data.h"
#include "fractal_generator.h"
#include "placement.h"
#include "noise.h"
#include <random>

class PatternCell {
public:
    PatternCell(std::mt19937& rng, PerlinNoise& noise);
    void generate(MapData& map, int numPlayers, PlacementMode placement,
                  const WaterParams& waterParams, const MetalParams& metalParams);

private:
    std::mt19937& rng_;
    PerlinNoise& noise_;
    FractalGenerator fractal_;
    PlacementStrategy placement_;

    void buildWalls(MapData& map);
    void fillStartingAreaTrees(MapData& map);
    void fillCommonAreaForest(MapData& map);
    void placeMetalDeposits(MapData& map, const MetalParams& params);

    // Check if a tile is inside a shaped starting area
    bool isInsideStartArea(const StartingArea& sa, int x, int y) const;
    bool isInsideAnyStartArea(const MapData& map, int x, int y) const;
};