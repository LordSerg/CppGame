#include "camera/Camera.h"
#include "core/Config.h"

Camera::Camera()
    : position(Config::MAP_WIDTH * 0.5f, Config::MAP_HEIGHT * 0.5f),
      zoomLevel(1.0f) {}

void Camera::pan(float dx, float dy) {
    position.x += dx;
    position.y += dy;
}

void Camera::zoom(float factor) {
    zoomLevel *= factor;
    if (zoomLevel < 0.1f) zoomLevel = 0.1f;
    if (zoomLevel > 10.0f) zoomLevel = 10.0f;
}

Vec2 Camera::screenToWorld(float sx, float sy, float viewportW, float viewportH) const {
    float halfW = (viewportW * 0.5f) / zoomLevel;
    float halfH = (viewportH * 0.5f) / zoomLevel;

    float worldX = position.x - halfW + (sx / viewportW) * 2.0f * halfW;
    float worldY = position.y - halfH + (sy / viewportH) * 2.0f * halfH;
    return {worldX, worldY};
}

Vec2 Camera::worldToScreen(Vec2 world, float viewportW, float viewportH) const {
    float halfW = (viewportW * 0.5f) / zoomLevel;
    float halfH = (viewportH * 0.5f) / zoomLevel;

    float sx = ((world.x - position.x + halfW) / (2.0f * halfW)) * viewportW;
    float sy = ((world.y - position.y + halfH) / (2.0f * halfH)) * viewportH;
    return {sx, sy};
}

void Camera::getViewBounds(float viewportW, float viewportH,
                            float& left, float& right, float& bottom, float& top) const {
    float halfW = (viewportW * 0.5f) / zoomLevel;
    float halfH = (viewportH * 0.5f) / zoomLevel;
    left = position.x - halfW;
    right = position.x + halfW;
    bottom = position.y - halfH;
    top = position.y + halfH;
}