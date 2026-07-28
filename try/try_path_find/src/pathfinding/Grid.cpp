#include "pathfinding/Grid.h"
#include "simulation/World.h"
#include <cmath>

Grid::Grid() : blocked_(Config::GRID_COLS * Config::GRID_ROWS, false) {}

void Grid::rebuild(const World& world) {
    std::fill(blocked_.begin(), blocked_.end(), false);
    for (const auto& b : world.getBarriers()) {
        // Mark all grid cells overlapping this barrier
        int minC, minR, maxC, maxR;
        worldToGrid(b.center.x - b.halfExtents.x, b.center.y - b.halfExtents.y, minC, minR);
        worldToGrid(b.center.x + b.halfExtents.x, b.center.y + b.halfExtents.y, maxC, maxR);
        minC = std::max(0, minC);
        minR = std::max(0, minR);
        maxC = std::min(Config::GRID_COLS - 1, maxC);
        maxR = std::min(Config::GRID_ROWS - 1, maxR);
        for (int r = minR; r <= maxR; ++r)
            for (int c = minC; c <= maxC; ++c)
                blocked_[r * Config::GRID_COLS + c] = true;
    }
}

bool Grid::isBlocked(int col, int row) const {
    if (col < 0 || col >= Config::GRID_COLS || row < 0 || row >= Config::GRID_ROWS)
        return true;
    return blocked_[row * Config::GRID_COLS + col];
}

void Grid::worldToGrid(float wx, float wy, int& col, int& row) {
    col = (int)(wx / Config::CELL_WIDTH);
    row = (int)(wy / Config::CELL_HEIGHT);
    col = std::max(0, std::min(col, Config::GRID_COLS - 1));
    row = std::max(0, std::min(row, Config::GRID_ROWS - 1));
}

void Grid::gridToWorld(int col, int row, float& wx, float& wy) {
    wx = col * Config::CELL_WIDTH + Config::CELL_WIDTH * 0.5f;
    wy = row * Config::CELL_HEIGHT + Config::CELL_HEIGHT * 0.5f;
}