#include "map_generator.h"
#include "pattern_cell.h"
#include "pattern_star.h"
#include "pattern_archipelago.h"
#include "fractal_generator.h"

MapGenerator::MapGenerator() : lastSeed_(0) {}

void MapGenerator::generate(MapData& map, MapSize size, MapPattern pattern,
                             int numPlayers, uint32_t seed) {
    lastSeed_ = seed;
    rng_.seed(seed);
    noise_.reseed(seed);

    int dim = (int)size;
    map.resize(dim, dim);
    map.clear();

    switch (pattern) {
        case MapPattern::Cell: {
            PatternCell cellGen(rng_, noise_);
            cellGen.generate(map, numPlayers);
            break;
        }
        case MapPattern::Star: {
            PatternStar starGen(rng_, noise_);
            starGen.generate(map, numPlayers);
            break;
        }
        case MapPattern::Archipelago: {
            PatternArchipelago archGen(rng_, noise_);
            archGen.generate(map, numPlayers);
            break;
        }
        default:
            break;
    }
}