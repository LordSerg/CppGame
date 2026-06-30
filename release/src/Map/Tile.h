#ifndef TILE_H
#define TILE_H

#include "../Utils/Math.h"

enum class TileType {
    GRASS,
    DIRT,
    STONE,
    WATER
};

class Tile {
public:
    Tile();
    Tile(int x, int y, TileType type);
    
    int GetX() const { return x; }
    int GetY() const { return y; }
    TileType GetType() const { return type; }
    
    bool IsWalkable() const;
    bool IsBuildable() const;
    bool HasTree() const { return hasTree; }
    bool HasRock() const { return hasRock; }
    bool HasRoad() const { return hasRoad; }
    
    void SetTree(bool value) { hasTree = value; }
    void SetRock(bool value) { hasRock = value; }
    void SetRoad(bool value) { hasRoad = value; }
    
    float GetMovementCost() const;
    
private:
    int x, y;
    TileType type;
    bool hasTree;
    bool hasRock;
    bool hasRoad;
};

#endif // TILE_H