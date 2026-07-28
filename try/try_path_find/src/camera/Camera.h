#pragma once

#include "simulation/Agent.h"

class Camera {
public:
    Camera();

    void pan(float dx, float dy);
    void zoom(float factor);

    // Convert screen coords to world coords
    Vec2 screenToWorld(float sx, float sy, float viewportWidth, float viewportHeight) const;
    // Convert world coords to screen coords
    Vec2 worldToScreen(Vec2 world, float viewportWidth, float viewportHeight) const;

    // Get the transform for rendering
    void getViewBounds(float viewportW, float viewportH,
                       float& left, float& right, float& bottom, float& top) const;

    Vec2 position; // center of view in world coords
    float zoomLevel;

    float panSpeed = 300.0f;
};