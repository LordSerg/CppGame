#pragma once

#include "simulation/Agent.h"
#include <vector>
#include <cmath>
#include <functional>

class SpatialHash {
public:
    SpatialHash(float cellSize);

    void clear();
    void reserve(int expectedAgents);
    void insert(int agentIndex, Vec2 position);

    // Zero-allocation query: calls callback for each found index
    void query(Vec2 position, float radius,
               const std::function<void(int)>& callback) const;

    // Convenience: fills a caller-provided vector (reuse it to avoid alloc)
    void query(Vec2 position, float radius, std::vector<int>& out) const;

private:
    // Open-addressing hash table for minimal overhead
    static constexpr int TABLE_SIZE = 4096; // power of 2
    static constexpr int BUCKET_CAP = 16;

    struct Bucket {
        int cellX, cellY;
        int count;
        int indices[BUCKET_CAP];
        bool occupied;
    };

    int hashCell(int cx, int cy) const;

    float cellSize_;
    float invCellSize_;
    Bucket table_[TABLE_SIZE];

    // Overflow storage for buckets that exceed BUCKET_CAP
    struct OverflowBucket {
        int cellX, cellY;
        std::vector<int> indices;
    };
    mutable std::vector<OverflowBucket> overflow_;
};