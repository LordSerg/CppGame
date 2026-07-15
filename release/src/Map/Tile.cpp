#include "Tile.h"

Tile::Tile()
    : x(0)
    , y(0)
    , type(TileType::GRASS)
    , walkable(true)
    , hasTree(false)
    , hasRock(false)
    , hasRoad(false)
    , movementCost(1.0f)
{
}

Tile::Tile(int x, int y, TileType type)
    : x(x)
    , y(y)
    , type(type)
    , walkable(true)
    , hasTree(false)
    , hasRock(false)
    , hasRoad(false)
    , movementCost(1.0f)
{
    walkable = (type != TileType::WATER);
    if (type == TileType::DIRT) {
        movementCost = 0.8f; // Paths are slightly slower
    }
}

bool Tile::IsBuildable() const {
    if (type == TileType::WATER) return false;
    if (hasTree) return false;
    if (hasRock) return false;
    return true;
}