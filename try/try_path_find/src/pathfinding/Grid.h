#pragma once

#include "core/Config.h"
#include <vector>

class World;

// Occupancy grid for grid-based pathfinders
class Grid {
public:
    Grid();

    void rebuild(const World& world);

    bool isBlocked(int col, int row) const;
    int getCols() const { return Config::GRID_COLS; }
    int getRows() const { return Config::GRID_ROWS; }

    // Convert between world and grid coords
    static void worldToGrid(float wx, float wy, int& col, int& row);
    static void gridToWorld(int col, int row, float& wx, float& wy);

private:
    std::vector<bool> blocked_; // GRID_COLS * GRID_ROWS
};