#pragma once

#include "map_data.h"
#include <vector>
#include <queue>
#include <set>

namespace Pathfinding {

// Check if all non-water tiles are connected
inline bool isFullyConnected(const MapData& map) {
    int w = map.getWidth();
    int h = map.getHeight();

    // Find first non-water tile
    int startX = -1, startY = -1;
    for (int y = 0; y < h && startX == -1; y++) {
        for (int x = 0; x < w && startX == -1; x++) {
            if (map.getTile(x, y) != TileType::Water) {
                startX = x;
                startY = y;
            }
        }
    }
    if (startX == -1) return true;

    // BFS
    std::vector<bool> visited(w * h, false);
    std::queue<std::pair<int,int>> q;
    q.push({startX, startY});
    visited[startY * w + startX] = true;
    int count = 1;

    while (!q.empty()) {
        auto [cx, cy] = q.front(); q.pop();
        int dx[] = {1, -1, 0, 0};
        int dy[] = {0, 0, 1, -1};
        for (int d = 0; d < 4; d++) {
            int nx = cx + dx[d];
            int ny = cy + dy[d];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h
                && !visited[ny * w + nx]
                && map.getTile(nx, ny) != TileType::Water) {
                visited[ny * w + nx] = true;
                count++;
                q.push({nx, ny});
            }
        }
    }

    // Count total non-water tiles
    int total = 0;
    for (int y = 0; y < h; y++)
        for (int x = 0; x < w; x++)
            if (map.getTile(x, y) != TileType::Water)
                total++;

    return count == total;
}

}