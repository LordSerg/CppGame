#include "spatial/SpatialHash.h"
#include <cstring>
#include <algorithm>

SpatialHash::SpatialHash(float cellSize)
    : cellSize_(cellSize), invCellSize_(1.0f / cellSize) {
    clear();
}

void SpatialHash::clear() {
    for (int i = 0; i < TABLE_SIZE; ++i) {
        table_[i].occupied = false;
        table_[i].count = 0;
    }
    overflow_.clear();
}

void SpatialHash::reserve(int /*expectedAgents*/) {
    overflow_.reserve(32);
}

int SpatialHash::hashCell(int cx, int cy) const {
    // Fast hash for 2D integer coordinates
    unsigned int h = (unsigned int)(cx * 73856093) ^ (unsigned int)(cy * 19349663);
    return (int)(h & (TABLE_SIZE - 1));
}

void SpatialHash::insert(int agentIndex, Vec2 position) {
    int cx = (int)std::floor(position.x * invCellSize_);
    int cy = (int)std::floor(position.y * invCellSize_);
    int h = hashCell(cx, cy);

    // Linear probing
    for (int probe = 0; probe < TABLE_SIZE; ++probe) {
        int idx = (h + probe) & (TABLE_SIZE - 1);
        Bucket& b = table_[idx];

        if (!b.occupied) {
            b.occupied = true;
            b.cellX = cx;
            b.cellY = cy;
            b.count = 1;
            b.indices[0] = agentIndex;
            return;
        }

        if (b.cellX == cx && b.cellY == cy) {
            if (b.count < BUCKET_CAP) {
                b.indices[b.count++] = agentIndex;
            } else {
                // Overflow
                for (auto& ov : overflow_) {
                    if (ov.cellX == cx && ov.cellY == cy) {
                        ov.indices.push_back(agentIndex);
                        return;
                    }
                }
                overflow_.push_back({cx, cy, {agentIndex}});
            }
            return;
        }
    }
}

void SpatialHash::query(Vec2 position, float radius,
                         const std::function<void(int)>& callback) const {
    int minCX = (int)std::floor((position.x - radius) * invCellSize_);
    int maxCX = (int)std::floor((position.x + radius) * invCellSize_);
    int minCY = (int)std::floor((position.y - radius) * invCellSize_);
    int maxCY = (int)std::floor((position.y + radius) * invCellSize_);

    for (int cy = minCY; cy <= maxCY; ++cy) {
        for (int cx = minCX; cx <= maxCX; ++cx) {
            int h = hashCell(cx, cy);

            for (int probe = 0; probe < TABLE_SIZE; ++probe) {
                int idx = (h + probe) & (TABLE_SIZE - 1);
                const Bucket& b = table_[idx];

                if (!b.occupied) break;

                if (b.cellX == cx && b.cellY == cy) {
                    for (int i = 0; i < b.count; ++i) {
                        callback(b.indices[i]);
                    }
                    // Check overflow
                    for (const auto& ov : overflow_) {
                        if (ov.cellX == cx && ov.cellY == cy) {
                            for (int i : ov.indices) callback(i);
                        }
                    }
                    break;
                }
            }
        }
    }
}

void SpatialHash::query(Vec2 position, float radius, std::vector<int>& out) const {
    out.clear();
    query(position, radius, [&out](int idx) { out.push_back(idx); });
}