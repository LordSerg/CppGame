#ifndef RENDERER_H
#define RENDERER_H

#include "../Utils/Math.h"
#include "Camera.h"
#include "Texture.h"
#include "Sprite.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Renderer {
public:
    Renderer();
    ~Renderer();
    
    bool Initialize(int width, int height);
    void Shutdown();
    
    void Clear(float r = 0.0f, float g = 0.0f, float b = 0.0f, float a = 1.0f);
    void SetViewport(int x, int y, int width, int height);
    
    // Camera
    Camera* GetCamera() { return camera.get(); }
    void SetCamera(Camera* cam);
    
    // Drawing
    void DrawSprite(const Sprite& sprite, const Vector2& position, 
                   const glm::vec3& color = glm::vec3(1.0f));
    void DrawRect(const Rect& rect, const glm::vec3& color);
    void DrawScreenRect(const Rect& rect, const glm::vec3& color);
    void DrawLine(const Vector2& start, const Vector2& end, 
                 const glm::vec3& color, float thickness = 1.0f);
    void DrawCircle(const Vector2& center, float radius, 
                   const glm::vec3& color);
    // Textured rect with sub-rect UV support (for sprite sheets)
    void DrawTexturedRect(Texture* texture, const Vector2& position, float width, float height,
                         const glm::vec3& color = glm::vec3(1.0f),
                         float uvX = 0.0f, float uvY = 0.0f, 
                         float uvW = 1.0f, float uvH = 1.0f);
    
    // Isometric conversion
    Vector2 WorldToScreen(const Vector2& worldPos) const;
    Vector2 ScreenToWorld(const Vector2& screenPos) const;
    
    // Texture management
    Texture* LoadTexture(const std::string& path);
    Texture* GetTexture(const std::string& name);
    
    // Batch rendering
    void BeginBatch();
    void EndBatch();
    
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    
private:
    int width;
    int height;
    
    std::unique_ptr<Camera> camera;
    std::unordered_map<std::string, std::unique_ptr<Texture>> textures;
    
    // OpenGL resources
    unsigned int quadVAO;
    unsigned int quadVBO;
    unsigned int shaderProgram;
    
    void InitializeQuad();
    void InitializeShaders();
    unsigned int CompileShader(const char* vertexSrc, const char* fragmentSrc);
};

#endif // RENDERER_H