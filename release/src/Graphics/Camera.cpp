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
}

void Camera::Move(const Vector2& delta) {
    position = position + delta;
}

void Camera::SetZoom(float z) {
    zoom = Math::Clamp(z, minZoom, maxZoom);
}

void Camera::Zoom(float delta) {
    zoom = Math::Clamp(zoom + delta, minZoom, maxZoom);
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
    float x = (screenPos.x - screenWidth / 2.0f) / zoom + position.x;
    float y = (screenPos.y - screenHeight / 2.0f) / zoom + position.y;
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
    
    position.x = Math::Clamp(position.x, 
                            bounds.x + halfWidth, 
                            bounds.x + bounds.width - halfWidth);
    position.y = Math::Clamp(position.y, 
                            bounds.y + halfHeight, 
                            bounds.y + bounds.height - halfHeight);
}