#pragma once

#include <glad/glad.h>
#include "map_data.h"

class Renderer {
public:
    Renderer();
    ~Renderer();

    void init();
    void shutdown();

    void renderMap(const MapData& map, float offsetX, float offsetY, float zoom,
                   bool showUnderground, bool showBarriers,
                   float viewportWidth, float viewportHeight);

private:
    GLuint shaderProgram_;
    GLuint vao_, vbo_;
    GLuint mapTexture_;
    GLuint metalTexture_;

    int texWidth_, texHeight_;

    void createShaders();
    void createBuffers();
    void updateMapTexture(const MapData& map, bool showBarriers);
    void updateMetalTexture(const MapData& map);

    GLuint compileShader(GLenum type, const char* source);
};