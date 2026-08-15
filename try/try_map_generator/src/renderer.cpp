#include "renderer.h"
#include <vector>
#include <algorithm>
#include <cmath>
#include <cstdio>

static const char* vertexShaderSource = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform vec2 uOffset;
uniform float uZoom;
uniform vec2 uViewport;

out vec2 TexCoord;

void main() {
    vec2 pos = (aPos * uZoom + uOffset) / (uViewport * 0.5);
    pos -= vec2(1.0, 1.0);
    pos.y = -pos.y;
    gl_Position = vec4(pos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

static const char* fragmentShaderSource = R"(
#version 330 core
in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D uMapTex;
uniform sampler2D uMetalTex;
uniform bool uShowMetal;

void main() {
    vec4 mapColor = texture(uMapTex, TexCoord);
    if (uShowMetal) {
        float metal = texture(uMetalTex, TexCoord).r;
        if (metal > 0.05) {
            // Blend metal as orange/copper color over the map
            vec3 metalColor = vec3(0.8, 0.5, 0.1) * metal;
            mapColor.rgb = mix(mapColor.rgb, metalColor, metal * 0.8);
        }
    }
    FragColor = mapColor;
}
)";

Renderer::Renderer()
    : shaderProgram_(0), vao_(0), vbo_(0),
      mapTexture_(0), metalTexture_(0),
      texWidth_(0), texHeight_(0) {}

Renderer::~Renderer() {
    shutdown();
}

void Renderer::init() {
    createShaders();
    createBuffers();
}

void Renderer::shutdown() {
    if (vao_) { glDeleteVertexArrays(1, &vao_); vao_ = 0; }
    if (vbo_) { glDeleteBuffers(1, &vbo_); vbo_ = 0; }
    if (mapTexture_) { glDeleteTextures(1, &mapTexture_); mapTexture_ = 0; }
    if (metalTexture_) { glDeleteTextures(1, &metalTexture_); metalTexture_ = 0; }
    if (shaderProgram_) { glDeleteProgram(shaderProgram_); shaderProgram_ = 0; }
}

GLuint Renderer::compileShader(GLenum type, const char* source) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        fprintf(stderr, "Shader compilation error: %s\n", infoLog);
    }
    return shader;
}

void Renderer::createShaders() {
    GLuint vert = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
    GLuint frag = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vert);
    glAttachShader(shaderProgram_, frag);
    glLinkProgram(shaderProgram_);

    GLint success;
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        char infoLog[512];
        glGetProgramInfoLog(shaderProgram_, 512, nullptr, infoLog);
        fprintf(stderr, "Shader link error: %s\n", infoLog);
    }

    glDeleteShader(vert);
    glDeleteShader(frag);
}

void Renderer::createBuffers() {
    // Quad: position (x, y), texcoord (u, v)
    float vertices[] = {
        0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 0.0f,  1.0f, 0.0f,
        1.0f, 1.0f,  1.0f, 1.0f,

        0.0f, 0.0f,  0.0f, 0.0f,
        1.0f, 1.0f,  1.0f, 1.0f,
        0.0f, 1.0f,  0.0f, 1.0f,
    };

    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void Renderer::updateMapTexture(const MapData& map, bool showBarriers) {
    int w = map.getWidth();
    int h = map.getHeight();

    if (w != texWidth_ || h != texHeight_) {
        if (mapTexture_) glDeleteTextures(1, &mapTexture_);

        glGenTextures(1, &mapTexture_);
        glBindTexture(GL_TEXTURE_2D, mapTexture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        texWidth_ = w;
        texHeight_ = h;
    }

    std::vector<uint8_t> pixels(w * h * 4);

    // Player colors for starting points
    static const uint8_t playerColors[8][3] = {
        {255, 50, 50},    // Red
        {50, 50, 255},    // Blue
        {50, 255, 50},    // Green
        {255, 255, 50},   // Yellow
        {255, 128, 0},    // Orange
        {200, 50, 200},   // Purple
        {0, 255, 255},    // Cyan
        {255, 180, 180},  // Pink
    };

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 4;
            TileType tile = map.getTile(x, y);

            uint8_t r, g, b, a;
            a = 255;

            switch (tile) {
                case TileType::Ground:
                    r = 139; g = 90; b = 43; // Brown
                    break;
                case TileType::Water:
                    r = 30; g = 100; b = 200; // Blue
                    break;
                case TileType::Tree:
                    // Green triangle symbolic - we use solid green
                    r = 34; g = 139; b = 34;
                    break;
                case TileType::Rock:
                    r = 150; g = 150; b = 150; // Gray
                    break;
                case TileType::StartingPoint: {
                    // Find which player
                    int pi = 0;
                    for (auto& sa : map.getStartingAreas()) {
                        int dx = x - sa.centerX;
                        int dy = y - sa.centerY;
                        if (dx * dx + dy * dy < 25) {
                            pi = sa.playerIndex;
                            break;
                        }
                    }
                    pi = std::clamp(pi, 0, 7);
                    r = playerColors[pi][0];
                    g = playerColors[pi][1];
                    b = playerColors[pi][2];
                    break;
                }
            }

            // Barrier overlay
            if (showBarriers && map.isBlocked(x, y)) {
                // Tint blocked tiles red
                r = (uint8_t)std::min(255, (int)r + 80);
                g = (uint8_t)(g / 2);
                b = (uint8_t)(b / 2);
                a = 220;
            }

            pixels[idx + 0] = r;
            pixels[idx + 1] = g;
            pixels[idx + 2] = b;
            pixels[idx + 3] = a;
        }
    }

    glBindTexture(GL_TEXTURE_2D, mapTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
}

void Renderer::updateMetalTexture(const MapData& map) {
    int w = map.getWidth();
    int h = map.getHeight();

    if (!metalTexture_) {
        glGenTextures(1, &metalTexture_);
        glBindTexture(GL_TEXTURE_2D, metalTexture_);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    }

    std::vector<uint8_t> pixels(w * h);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            pixels[y * w + x] = (uint8_t)(std::clamp(map.getMetal(x, y), 0.0f, 1.0f) * 255.0f);
        }
    }

    glBindTexture(GL_TEXTURE_2D, metalTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, pixels.data());
}

void Renderer::renderMap(const MapData& map, float offsetX, float offsetY, float zoom,
                          bool showUnderground, bool showBarriers,
                          float viewportWidth, float viewportHeight) {
    if (map.getWidth() == 0 || map.getHeight() == 0) return;

    updateMapTexture(map, showBarriers);
    updateMetalTexture(map);

    glUseProgram(shaderProgram_);

    // Set uniforms
    glUniform2f(glGetUniformLocation(shaderProgram_, "uOffset"), offsetX, offsetY);
    glUniform1f(glGetUniformLocation(shaderProgram_, "uZoom"), zoom);
    glUniform2f(glGetUniformLocation(shaderProgram_, "uViewport"), viewportWidth, viewportHeight);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uShowMetal"), showUnderground ? 1 : 0);

    // Bind textures
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mapTexture_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uMapTex"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, metalTexture_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uMetalTex"), 1);

    // Update quad to map dimensions
    float mapW = (float)map.getWidth();
    float mapH = (float)map.getHeight();

    float vertices[] = {
        0.0f, 0.0f,  0.0f, 0.0f,
        mapW, 0.0f,  1.0f, 0.0f,
        mapW, mapH,  1.0f, 1.0f,

        0.0f, 0.0f,  0.0f, 0.0f,
        mapW, mapH,  1.0f, 1.0f,
        0.0f, mapH,  0.0f, 1.0f,
    };

    glBindVertexArray(vao_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glUseProgram(0);
}