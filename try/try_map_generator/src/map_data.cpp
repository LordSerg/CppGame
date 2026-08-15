#include "map_data.h"

MapData::MapData() : width_(0), height_(0) {}

void MapData::resize(int width, int height) {
    width_ = width;
    height_ = height;
    tiles_.resize(width * height, TileType::Ground);
    metalMap_.resize(width * height, 0.0f);
}

void MapData::clear() {
    std::fill(tiles_.begin(), tiles_.end(), TileType::Ground);
    std::fill(metalMap_.begin(), metalMap_.end(), 0.0f);
    startingAreas_.clear();
}

TileType MapData::getTile(int x, int y) const {
    if (!inBounds(x, y)) return TileType::Ground;
    return tiles_[y * width_ + x];
}

void MapData::setTile(int x, int y, TileType type) {
    if (!inBounds(x, y)) return;
    tiles_[y * width_ + x] = type;
}

float MapData::getMetal(int x, int y) const {
    if (!inBounds(x, y)) return 0.0f;
    return metalMap_[y * width_ + x];
}

void MapData::setMetal(int x, int y, float value) {
    if (!inBounds(x, y)) return;
    metalMap_[y * width_ + x] = value;
}

bool MapData::isBlocked(int x, int y) const {
    if (!inBounds(x, y)) return true;
    TileType t = getTile(x, y);
    return t == TileType::Water || t == TileType::Tree || t == TileType::Rock;
}

bool MapData::inBounds(int x, int y) const {
    return x >= 0 && x < width_ && y >= 0 && y < height_;
}

void MapData::addStartingArea(const StartingArea& area) {
    startingAreas_.push_back(area);
}

void MapData::clearStartingAreas() {
    startingAreas_.clear();
}