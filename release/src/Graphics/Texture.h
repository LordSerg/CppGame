#ifndef TEXTURE_H
#define TEXTURE_H

#include <string>

class Texture {
public:
    Texture();
    ~Texture();
    
    bool Load(const std::string& path);
    void Unload();
    
    unsigned int GetID() const { return textureID; }
    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    
private:
    unsigned int textureID;
    int width;
    int height;
    int channels;
};

#endif // TEXTURE_H