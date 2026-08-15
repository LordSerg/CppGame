#pragma once

#include "map_data.h"
#include "noise.h"
#include <random>
#include <vector>
#include <utility>

struct PlacementResult {
    std::vector<StartingArea> areas;
};

class PlacementStrategy {
public:
    PlacementStrategy(std::mt19937& rng, PerlinNoise& noise);

    PlacementResult place(MapData& map, int numPlayers, PlacementMode mode);

private:
    std::mt19937& rng_;
    PerlinNoise& noise_;

    // Strategy implementations
    PlacementResult placeCircle(MapData& map, int numPlayers);
    PlacementResult placeOrganic(MapData& map, int numPlayers);
    PlacementResult placeVoronoi(MapData& map, int numPlayers);
    PlacementResult placeSpiral(MapData& map, int numPlayers);
    PlacementResult placeClustered(MapData& map, int numPlayers);

    // Helpers
    int baseRadius(int mapSize) const;
    void ensureMinDistance(std::vector<std::pair<int,int>>& positions, int minDist,
                           int mapW, int mapH, int margin);

    // Organic shape generation using noise-distorted boundary
    void generateOrganicShape(StartingArea& area, MapData& map, float noiseScale,
                               float distortion);

    // Voronoi cell computation
    void computeVoronoiCells(std::vector<StartingArea>& areas, MapData& map,
                              int maxCellRadius);

    // Compute boundary tiles for a shaped area
    void computeBoundary(StartingArea& area, MapData& map, int thickness);

    // Flood fill to find connected organic shape
    void floodFillShape(StartingArea& area, MapData& map, float noiseScale,
                         float threshold, int maxTiles);
};