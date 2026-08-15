#include "map_generator.h"
#include "pattern_cell.h"
#include "pattern_star.h"
#include "pattern_archipelago.h"

MapGenerator::MapGenerator() : lastSeed_(0) {}

void MapGenerator::generate(MapData& map, const GenerationParams& params) {
    lastSeed_ = params.seed;
    rng_.seed(params.seed);
    noise_.reseed(params.seed);

    int dim = (int)params.size;
    map.resize(dim, dim);
    map.clear();

    switch (params.pattern) {
        case MapPattern::Cell: {
            PatternCell cellGen(rng_, noise_);
            cellGen.generate(map, params.numPlayers, params.placement,
                             params.water, params.metal);
            break;
        }
        case MapPattern::Star: {
            PatternStar starGen(rng_, noise_);
            starGen.generate(map, params.numPlayers, params.placement,
                             params.water, params.metal);
            break;
        }
        case MapPattern::Archipelago: {
            PatternArchipelago archGen(rng_, noise_);
            archGen.generate(map, params.numPlayers, params.placement,
                             params.water, params.metal);
            break;
        }
        default:
            break;
    }
}