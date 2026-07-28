#include "spatial/SpatialHash.h"
#include <cmath>

SpatialHash::SpatialHash(float cellSize) : cellSize_(cellSize) {}

void SpatialHash::clear() {
    cells_.clear();
}

SpatialHash::CellKey SpatialHash::toCell(Vec2 pos) const {
    return {(int)std::floor(pos.x / cellSize_),
            (int)std::floor(pos.y / cellSize_)};
}

void SpatialHash::insert(int agentIndex, Vec2 position) {
    CellKey key = toCell(position);
    cells_[key].push_back(agentIndex);
}

std::vector<int> SpatialHash::query(Vec2 position, float radius) const {
    std::vector<int> result;
    int minCX = (int)std::floor((position.x - radius) / cellSize_);
    int maxCX = (int)std::floor((position.x + radius) / cellSize_);
    int minCY = (int)std::floor((position.y - radius) / cellSize_);
    int maxCY = (int)std::floor((position.y + radius) / cellSize_);

    for (int cx = minCX; cx <= maxCX; ++cx) {
        for (int cy = minCY; cy <= maxCY; ++cy) {
            auto it = cells_.find({cx, cy});
            if (it != cells_.end()) {
                for (int idx : it->second) {
                    result.push_back(idx);
                }
            }
        }
    }
    return result;
}