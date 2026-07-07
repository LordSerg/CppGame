#include "Camera.h"
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(int screenWidth, int screenHeight)
    : position(0, 0)
    , zoom(1.0f)
    , screenWidth(screenWidth)
    , screenHeight(screenHeight)
    , hasBounds(false)
    , minZoom(0.5f)
    , maxZoom(2.0f)
{
}

void Camera::Update(float deltaTime) {
    if (hasBounds) {
        ClampToBounds();
    }
}

void Camera::SetPosition(const Vector2& pos) {
    position = pos;
    if (hasBounds) {
        ClampToBounds();
    }
}

void Camera::Move(const Vector2& delta) {
    position = position + delta;
    if (hasBounds) {
        ClampToBounds();
    }
}

void Camera::SetZoom(float z) {
    zoom = Math::Clamp(z, minZoom, maxZoom);
    if (hasBounds) {
        ClampToBounds();
    }
}

void Camera::Zoom(float delta) {
    zoom = Math::Clamp(zoom + delta, minZoom, maxZoom);
    if (hasBounds) {
        ClampToBounds();
    }
}

glm::mat4 Camera::GetProjectionMatrix() const {
    float halfWidth = screenWidth / (2.0f * zoom);
    float halfHeight = screenHeight / (2.0f * zoom);
    return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, -1.0f, 1.0f);
}

glm::mat4 Camera::GetViewMatrix() const {
    return glm::translate(glm::mat4(1.0f), glm::vec3(-position.x, -position.y, 0.0f));
}

Vector2 Camera::ScreenToWorld(const Vector2& screenPos) const {
    // GLFW gives Y=0 at top, Y=screenHeight at bottom.
    // Our world has Y increasing upward, so flip Y.
    float flippedY = screenHeight - screenPos.y;
    float x = (screenPos.x - screenWidth / 2.0f) / zoom + position.x;
    float y = (flippedY - screenHeight / 2.0f) / zoom + position.y;
    return Vector2(x, y);
}

Vector2 Camera::WorldToScreen(const Vector2& worldPos) const {
    float x = (worldPos.x - position.x) * zoom + screenWidth / 2.0f;
    float y = (worldPos.y - position.y) * zoom + screenHeight / 2.0f;
    return Vector2(x, y);
}

void Camera::SetBounds(const Rect& b) {
    bounds = b;
    hasBounds = true;
}

void Camera::ClampToBounds() {
    if (!hasBounds) return;
    
    float halfWidth = screenWidth / (2.0f * zoom);
    float halfHeight = screenHeight / (2.0f * zoom);
    
    // At least 1 tile (32 world units) of the map must always be visible
    const float minOverlap = 32.0f;
    
    // Calculate valid X range for camera position
    // Left edge: viewport right edge must be at least minOverlap into the map
    float minX = bounds.x + minOverlap - halfWidth;
    // Right edge: viewport left edge must be at least minOverlap before the map end
    float maxX = bounds.x + bounds.width - minOverlap + halfWidth;
    
    // If viewport is wider than the map, center the map in the viewport
    if (minX > maxX) {
        position.x = bounds.x + bounds.width / 2.0f;
    } else {
        position.x = Math::Clamp(position.x, minX, maxX);
    }
    
    // Same for Y axis
    float minY = bounds.y + minOverlap - halfHeight;
    float maxY = bounds.y + bounds.height - minOverlap + halfHeight;
    
    if (minY > maxY) {
        position.y = bounds.y + bounds.height / 2.0f;
    } else {
        position.y = Math::Clamp(position.y, minY, maxY);
    }
}
