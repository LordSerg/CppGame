#include "Tile.h"

Tile::Tile()
    : x(0)
    , y(0)
    , type(TileType::GRASS)
    , hasTree(false)
    , hasRock(false)
    , hasRoad(false)
{
}

Tile::Tile(int x, int y, TileType type)
    : x(x)
    , y(y)
    , type(type)
    , hasTree(false)
    , hasRock(false)
    , hasRoad(false)
{
}

bool Tile::IsWalkable() const {
    if (type == TileType::WATER) return false;
    if (hasRock) return false;
    return true;
}

bool Tile::IsBuildable() const {
    if (type == TileType::WATER) return false;
    if (hasTree) return false;
    if (hasRock) return false;
    return true;
}

float Tile::GetMovementCost() const {
    if (!IsWalkable()) return -1.0f;
    
    float cost = 1.0f;
    
    switch (type) {
        case TileType::GRASS:
            cost = 1.0f;
            break;
        case TileType::DIRT:
            cost = 1.1f;
            break;
        case TileType::STONE:
            cost = 1.3f;
            break;
        default:
            cost = 1.0f;
    }
    
    if (hasRoad) {
        cost *= 0.7f; // Roads make movement faster
    }
    
    if (hasTree) {
        cost *= 1.5f; // Trees slow movement
    }
    
    return cost;
}