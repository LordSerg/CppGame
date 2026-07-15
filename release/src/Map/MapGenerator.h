#ifndef MAP_GENERATOR_H
#define MAP_GENERATOR_H

#include "../Utils/Math.h"
#include "Tile.h"
#include <vector>
#include <random>
#include <cstdint>
#include <functional>

enum class MapSize;
class Map;

// Perlin noise implementation
class PerlinNoise {
public:
    PerlinNoise(uint32_t seed);

    // Returns value in range [-1, 1]
    double Noise(double x, double y) const;

    // Fractal Brownian Motion - layered noise for more natural results
    // Returns value in range approximately [-1, 1]
    double FBM(double x, double y, int octaves, double lacunarity = 2.0,
               double persistence = 0.5) const;

private:
    std::vector<int> permutation;

    double Fade(double t) const;
    double Lerp(double t, double a, double b) const;
    double Grad(int hash, double x, double y) const;
};

// Represents a starting zone for a player
struct StartingZone {
    int playerId;
    Point2D center;          // Center tile of the starting area
    int radius;              // Radius of cleared area around start
    Point2D hutPosition;     // Where the hut goes
    Point2D peasantPosition; // Where the peasant spawns
    std::vector<Point2D> guaranteedTreePositions;  // Nearby trees
    std::vector<Point2D> guaranteedStonePositions; // Nearby stones
    Point2D nearestWaterAccess; // Closest water tile
};

// Configuration for map generation
struct MapGenConfig {
    int width;
    int height;
    uint32_t seed;
    int numPlayers;

    // Terrain noise parameters
    double terrainScale;        // Base scale for terrain noise
    double forestScale;         // Base scale for forest density noise
    double forestDensity;       // 0.0 - 1.0, how much of land is forested
    double treeDensity;         // Probability of tree on a forest tile

    // Water parameters
    int numRivers;              // Number of rivers to generate
    int numLakes;               // Number of lakes
    double lakeMinRadius;
    double lakeMaxRadius;
    double riverWidth;          // Width of rivers in tiles

    // Starting zone parameters
    int startingZoneRadius;     // Clear area around each player start
    int startingResourceRadius; // How far resources can be from start
    int minTreesPerStart;       // Minimum guaranteed trees near start
    int minStonesPerStart;      // Minimum guaranteed stones near start
    int waterAccessMaxDist;     // Max distance to water from start

    // Path parameters
    double pathWidth;           // Width of paths through forests
    int numMainPaths;           // Number of main paths connecting areas

    // Minimum distances
    int minPlayerDistance;      // Minimum distance between player starts

    static MapGenConfig GetDefault(MapSize size, int numPlayers);
};

class MapGenerator {
public:
    MapGenerator(uint32_t seed = 0);
    ~MapGenerator() = default;

    // Main generation function - populates the Map object
    // Returns starting zones for each player
    std::vector<StartingZone> Generate(Map* map, int numPlayers,
                                        MapSize size);

    // Set specific seed
    void SetSeed(uint32_t seed);

private:
    uint32_t seed;
    std::mt19937 rng;
    MapGenConfig config;

    // Internal generation buffers (same size as map)
    int mapWidth;
    int mapHeight;
    std::vector<std::vector<float>> heightMap;
    std::vector<std::vector<float>> moistureMap;
    std::vector<std::vector<float>> forestMap;
    std::vector<std::vector<bool>> waterMask;
    std::vector<std::vector<bool>> treeMask;
    std::vector<std::vector<bool>> stoneMask;
    std::vector<std::vector<bool>> pathMask;

    // Generation phases (called in order)
    void InitializeBuffers();
    void GenerateHeightMap(PerlinNoise& noise);
    void GenerateMoistureMap(PerlinNoise& noise);
    void GenerateForestMap(PerlinNoise& noise);

    // Water features
    void GenerateLakes(PerlinNoise& noise);
    void GenerateRivers();
    void SmoothWaterEdges();

    // Starting positions
    std::vector<StartingZone> PlaceStartingZones(int numPlayers);
    bool IsValidStartPosition(const Point2D& pos,
                               const std::vector<StartingZone>& existing) const;
    void ClearStartingArea(StartingZone& zone);
    void EnsureWaterAccess(StartingZone& zone);
    void PlaceStartingResources(StartingZone& zone);

    // Paths
    void GenerateMainPaths(const std::vector<StartingZone>& zones);
    void CarvePathBetween(const Point2D& from, const Point2D& to, double width);
    std::vector<Point2D> FindPathAStar(const Point2D& from, const Point2D& to,
                                        bool allowWater = false) const;

    // Object placement
    void PlaceTrees();
    void PlaceStones(PerlinNoise& noise);

    // Apply everything to the actual Map object
    void ApplyToMap(Map* map, const std::vector<StartingZone>& zones);

    // River helpers
    struct RiverSegment {
        Point2D start;
        Point2D end;
    };
    void TraceRiver(Point2D start, int direction);
    Point2D FindRiverSource() const;
    Point2D FindLakePosition(PerlinNoise& noise) const;

    // Utility
    float DistanceBetween(const Point2D& a, const Point2D& b) const;
    bool IsInBounds(int x, int y) const;
    void FloodFillWater(int x, int y, float radius, float irregularity,
                         PerlinNoise& noise);
    std::vector<Point2D> GetTilesInRadius(const Point2D& center,
                                           int radius) const;
    std::vector<Point2D> GetLandTilesInRadius(const Point2D& center,
                                               int radius) const;
    Point2D FindNearestWater(const Point2D& from, int maxSearch) const;
    void DigChannelToWater(const Point2D& from, const Point2D& waterTile,
                            double width);

    // Connectivity verification
    bool VerifyAllStartsConnected(const std::vector<StartingZone>& zones) const;
    bool BFSPathExists(const Point2D& from, const Point2D& to) const;
};

#endif // MAP_GENERATOR_H