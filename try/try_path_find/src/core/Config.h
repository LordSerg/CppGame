#pragma once

namespace Config {
    // Map dimensions (change these to resize the world)
    constexpr float MAP_WIDTH = 500.0f;
    constexpr float MAP_HEIGHT = 500.0f;

    // Grid resolution (for grid-based pathfinding)
    constexpr int GRID_COLS = 100;
    constexpr int GRID_ROWS = 100;
    constexpr float CELL_WIDTH = MAP_WIDTH / GRID_COLS;
    constexpr float CELL_HEIGHT = MAP_HEIGHT / GRID_ROWS;

    // Default agent properties
    constexpr float DEFAULT_AGENT_RADIUS = 3.0f;
    constexpr float DEFAULT_AGENT_SPEED = 50.0f;

    // Default barrier size
    constexpr float DEFAULT_BARRIER_HALF_SIZE = 8.0f;

    // Spatial hash cell size (should be >= 2 * max agent interaction radius)
    constexpr float SPATIAL_HASH_CELL = 20.0f;

    // Rendering
    constexpr int WINDOW_WIDTH = 1280;
    constexpr int WINDOW_HEIGHT = 720;
    constexpr float UI_PANEL_WIDTH = 280.0f;

    // Simulation
    constexpr float FIXED_TIMESTEP = 1.0f / 60.0f;
    constexpr int CIRCLE_SEGMENTS = 16;

    // Agent path following
    constexpr float WAYPOINT_REACH_DIST = 5.0f;
    constexpr float ARRIVAL_SLOW_RADIUS = 20.0f;
}