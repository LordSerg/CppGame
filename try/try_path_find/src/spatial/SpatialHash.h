#pragma once

#include "simulation/Agent.h"
#include <unordered_map>
#include <cstddef>
#include <vector>

class SpatialHash {
public:
    SpatialHash(float cellSize);

    void clear();
    void insert(int agentIndex, Vec2 position);
    std::vector<int> query(Vec2 position, float radius) const;

private:
    struct CellKey {
        int x, y;
        bool operator==(const CellKey& o) const { return x == o.x && y == o.y; }
    };

    struct CellKeyHash {
        size_t operator()(const CellKey& k) const {
            return std::hash<int>()(k.x) ^ (std::hash<int>()(k.y) << 16);
        }
    };

    CellKey toCell(Vec2 pos) const;

    float cellSize_;
    std::unordered_map<CellKey, std::vector<int>, CellKeyHash> cells_;
};