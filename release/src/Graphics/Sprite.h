#ifndef SPRITE_H
#define SPRITE_H

#include "Texture.h"
#include "../Utils/Math.h"

class Sprite {
public:
    Sprite();
    Sprite(Texture* texture, const Rect& srcRect);
    
    void SetTexture(Texture* tex) { texture = tex; }
    void SetSourceRect(const Rect& rect) { sourceRect = rect; }
    void SetSize(float w, float h) { width = w; height = h; }
    
    Texture* GetTexture() const { return texture; }
    Rect GetSourceRect() const { return sourceRect; }
    float GetWidth() const { return width; }
    float GetHeight() const { return height; }
    
private:
    Texture* texture;
    Rect sourceRect;
    float width;
    float height;
};

#endif // SPRITE_H