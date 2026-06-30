#include "Sprite.h"

Sprite::Sprite()
    : texture(nullptr)
    , sourceRect(0, 0, 0, 0)
    , width(0)
    , height(0)
{
}

Sprite::Sprite(Texture* tex, const Rect& srcRect)
    : texture(tex)
    , sourceRect(srcRect)
    , width(srcRect.width)
    , height(srcRect.height)
{
}