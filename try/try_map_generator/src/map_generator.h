#pragma once

#include "map_data.h"
#include "fractal_generator.h"
#include "noise.h"
#include <random>
#include <cstdint>

struct GenerationParams {
    MapSize size = MapSize::Small;
    MapPattern pattern = MapPattern::Cell;
    PlacementMode placement = PlacementMode::Circle;
    int numPlayers = 2;
    uint32_t seed = 12345;
    WaterParams water;
    MetalParams metal;
};

class MapGenerator {
public:
    MapGenerator();

    void generate(MapData& map, const GenerationParams& params);

    uint32_t getLastSeed() const { return lastSeed_; }

private:
    uint32_t lastSeed_;
    std::mt19937 rng_;
    PerlinNoise noise_;
};