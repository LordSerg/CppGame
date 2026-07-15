#ifndef TILE_H
#define TILE_H

#include "../Utils/Math.h"

enum class TileType {
    GRASS,
    DIRT,
    WATER
};

class Tile {
public:
    Tile();
    Tile(TileType type);
    Tile(int x, int y, TileType type);
    ~Tile() = default;
    
    int GetX() const { return x; }
    int GetY() const { return y; }

    TileType GetType() const { return type; }
    void SetType(TileType newType) { type = newType; }
    
    bool IsWalkable() const { return walkable; }
    void SetWalkable(bool value) { walkable = value; }

    bool IsBuildable() const;
    
    bool HasTree() const { return hasTree; }
    void SetHasTree(bool value) { hasTree = value; }

    bool HasRock() const { return hasRock; }
    void SetHasRock(bool value) { hasRock = value; }


    bool HasRoad() const { return hasRoad; }
    void SetRoad(bool value) { hasRoad = value; }
    
    void SetTree(bool value) { hasTree = value; }
    void SetRock(bool value) { hasRock = value; }
    
    float GetMovementCost() const { return movementCost; }
    void SetMovementCost(float cost) { movementCost = cost; }
    
private:
    int x, y;
    TileType type;
    bool walkable;
    bool hasTree;
    bool hasRock;
    bool hasRoad;
    float movementCost;
};

#endif // TILE_H