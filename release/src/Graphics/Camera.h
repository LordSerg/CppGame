#ifndef CAMERA_H
#define CAMERA_H

#include "../Utils/Math.h"
#include <glm/glm.hpp>

class Camera {
public:
    Camera(int screenWidth, int screenHeight);
    
    void Update(float deltaTime);
    
    void SetPosition(const Vector2& pos);
    void Move(const Vector2& delta);
    
    void SetZoom(float z);
    void Zoom(float delta);
    
    Vector2 GetPosition() const { return position; }
    float GetZoom() const { return zoom; }
    
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewMatrix() const;
    
    // Screen/World conversion
    Vector2 ScreenToWorld(const Vector2& screenPos) const;
    Vector2 WorldToScreen(const Vector2& worldPos) const;
    
    // Camera bounds
    void SetBounds(const Rect& bounds);
    void ClampToBounds();
    
private:
    Vector2 position;
    float zoom;
    
    int screenWidth;
    int screenHeight;
    
    Rect bounds;
    bool hasBounds;
    
    float minZoom;
    float maxZoom;
};

#endif // CAMERA_H