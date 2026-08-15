#pragma once

#include "map_data.h"
#include "noise.h"
#include <random>
#include <cstdint>

class MapGenerator {
public:
    MapGenerator();

    void generate(MapData& map, MapSize size, MapPattern pattern,
                  int numPlayers, uint32_t seed);

    uint32_t getLastSeed() const { return lastSeed_; }

private:
    uint32_t lastSeed_;
    std::mt19937 rng_;
    PerlinNoise noise_;
};