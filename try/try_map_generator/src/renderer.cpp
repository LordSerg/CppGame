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
    // Transform: world position -> screen pixel -> NDC
    vec2 screenPos = aPos * uZoom + uOffset;
    vec2 ndc = (screenPos / uViewport) * 2.0 - 1.0;
    ndc.y = -ndc.y;
    gl_Position = vec4(ndc, 0.0, 1.0);
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
        if (metal > 0.03) {
            // Frozen underground river of metal: copper/amber glow
            float glow = metal * metal; // nonlinear for more contrast
            vec3 metalColor = mix(
                vec3(0.6, 0.35, 0.05),  // dark copper
                vec3(1.0, 0.7, 0.15),   // bright gold
                glow
            );
            mapColor.rgb = mix(mapColor.rgb, metalColor, metal * 0.85);
        }
    }
    FragColor = mapColor;
}
)";

Renderer::Renderer()
    : shaderProgram_(0), vao_(0), vbo_(0),
      mapTexture_(0), metalTexture_(0),
      texWidth_(0), texHeight_(0),
      texturesDirty_(true), lastShowBarriers_(false) {}

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

void Renderer::invalidateTextures() {
    texturesDirty_ = true;
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

    bool needsRealloc = (w != texWidth_ || h != texHeight_);

    if (needsRealloc) {
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

    static const uint8_t playerColors[8][3] = {
        {255, 50, 50},
        {50, 50, 255},
        {50, 255, 50},
        {255, 255, 50},
        {255, 128, 0},
        {200, 50, 200},
        {0, 255, 255},
        {255, 180, 180},
    };

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = (y * w + x) * 4;
            TileType tile = map.getTile(x, y);

            uint8_t r, g, b, a;
            a = 255;

            switch (tile) {
                case TileType::Ground:
                    r = 139; g = 90; b = 43;
                    break;
                case TileType::Water:
                    r = 30; g = 100; b = 200;
                    break;
                case TileType::Tree:
                    r = 34; g = 139; b = 34;
                    break;
                case TileType::Rock:
                    r = 150; g = 150; b = 150;
                    break;
                case TileType::StartingPoint: {
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

            // Expand starting point marker to be more visible
            if (tile != TileType::StartingPoint) {
                for (auto& sa : map.getStartingAreas()) {
                    int dx = x - sa.centerX;
                    int dy = y - sa.centerY;
                    int markerSize = std::max(3, sa.radius / 8);
                    if (dx * dx + dy * dy < markerSize * markerSize) {
                        int pi = std::clamp(sa.playerIndex, 0, 7);
                        r = playerColors[pi][0];
                        g = playerColors[pi][1];
                        b = playerColors[pi][2];
                        break;
                    }
                }
            }

            if (showBarriers && map.isBlocked(x, y)) {
                r = (uint8_t)std::min(255, (int)r + 100);
                g = (uint8_t)(g / 3);
                b = (uint8_t)(b / 3);
                a = 230;
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

    // Update textures only when needed
    if (texturesDirty_ || showBarriers != lastShowBarriers_) {
        updateMapTexture(map, showBarriers);
        updateMetalTexture(map);
        texturesDirty_ = false;
        lastShowBarriers_ = showBarriers;
    }

    glUseProgram(shaderProgram_);

    glUniform2f(glGetUniformLocation(shaderProgram_, "uOffset"), offsetX, offsetY);
    glUniform1f(glGetUniformLocation(shaderProgram_, "uZoom"), zoom);
    glUniform2f(glGetUniformLocation(shaderProgram_, "uViewport"), viewportWidth, viewportHeight);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uShowMetal"), showUnderground ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, mapTexture_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uMapTex"), 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, metalTexture_);
    glUniform1i(glGetUniformLocation(shaderProgram_, "uMetalTex"), 1);

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