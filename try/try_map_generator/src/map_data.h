#pragma once

#include <vector>
#include <cstdint>
#include <string>

enum class TileType : uint8_t {
    Ground = 0,
    Water,
    Tree,
    Rock,
    StartingPoint
};

enum class MapSize : int {
    Small = 500,
    Mid = 1000,
    Big = 2000,
    Mammoth = 4000
};

enum class MapPattern : int {
    Cell = 0,
    Star,
    Archipelago,
    COUNT
};

enum class PlacementMode : int {
    Circle = 0,
    Organic,
    Voronoi,
    Spiral,
    Clustered,
    COUNT
};

inline const char* MapPatternName(MapPattern p) {
    switch (p) {
        case MapPattern::Cell: return "Cell";
        case MapPattern::Star: return "Star";
        case MapPattern::Archipelago: return "Archipelago";
        default: return "Unknown";
    }
}

inline const char* MapSizeName(MapSize s) {
    switch (s) {
        case MapSize::Small: return "Small (500x500)";
        case MapSize::Mid: return "Mid (1000x1000)";
        case MapSize::Big: return "Big (2000x2000)";
        case MapSize::Mammoth: return "Mammoth (4000x4000)";
        default: return "Unknown";
    }
}

inline const char* PlacementModeName(PlacementMode m) {
    switch (m) {
        case PlacementMode::Circle: return "Circle (Symmetric)";
        case PlacementMode::Organic: return "Organic (Natural shapes)";
        case PlacementMode::Voronoi: return "Voronoi (Territory cells)";
        case PlacementMode::Spiral: return "Spiral (Golden ratio)";
        case PlacementMode::Clustered: return "Clustered (Teams)";
        default: return "Unknown";
    }
}

struct StartingArea {
    int centerX, centerY;
    int radius;
    int playerIndex;

    // Organic/Voronoi shape: per-tile mask of which tiles belong to this area
    // If empty, use circular radius
    std::vector<std::pair<int,int>> shapeTiles;

    // Boundary tiles for wall building (Cell pattern)
    std::vector<std::pair<int,int>> boundaryTiles;

    bool hasShape() const { return !shapeTiles.empty(); }
};

class MapData {
public:
    MapData();
    ~MapData() = default;

    void resize(int width, int height);
    void clear();

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    TileType getTile(int x, int y) const;
    void setTile(int x, int y, TileType type);

    float getMetal(int x, int y) const;
    void setMetal(int x, int y, float value);

    bool isBlocked(int x, int y) const;
    bool inBounds(int x, int y) const;

    const std::vector<StartingArea>& getStartingAreas() const { return startingAreas_; }
    std::vector<StartingArea>& getStartingAreasMut() { return startingAreas_; }
    void addStartingArea(const StartingArea& area);
    void clearStartingAreas();

    const std::vector<TileType>& getTiles() const { return tiles_; }
    const std::vector<float>& getMetalMap() const { return metalMap_; }

private:
    int width_, height_;
    std::vector<TileType> tiles_;
    std::vector<float> metalMap_;
    std::vector<StartingArea> startingAreas_;
};