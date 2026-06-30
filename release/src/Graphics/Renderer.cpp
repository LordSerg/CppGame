#include "Renderer.h"
#include <glad/glad.h>
#include <iostream>

// Simple shader sources
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 projection;
uniform mat4 model;

void main() {
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";

const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;
uniform vec3 color;
uniform bool useTexture;

void main() {
    if (useTexture) {
        vec4 texColor = texture(texture1, TexCoord);
        FragColor = texColor * vec4(color, 1.0);
    } else {
        FragColor = vec4(color, 1.0);
    }
}
)";

Renderer::Renderer() 
    : width(0)
    , height(0)
    , quadVAO(0)
    , quadVBO(0)
    , shaderProgram(0)
{
}

Renderer::~Renderer() {
    Shutdown();
}

bool Renderer::Initialize(int w, int h) {
    width = w;
    height = h;
    
    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }
    
    // OpenGL settings
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialize camera
    camera = std::make_unique<Camera>(width, height);
    
    // Initialize rendering resources
    InitializeShaders();
    InitializeQuad();
    
    return true;
}

void Renderer::Shutdown() {
    if (quadVAO) {
        glDeleteVertexArrays(1, &quadVAO);
    }
    if (quadVBO) {
        glDeleteBuffers(1, &quadVBO);
    }
    if (shaderProgram) {
        glDeleteProgram(shaderProgram);
    }
}

void Renderer::Clear(float r, float g, float b, float a) {
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::SetViewport(int x, int y, int w, int h) {
    glViewport(x, y, w, h);
}

void Renderer::DrawSprite(const Sprite& sprite, const Vector2& position, 
                         const glm::vec3& color) {
    glUseProgram(shaderProgram);
    
    // Create model matrix
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(position.x, position.y, 0.0f));
    model = glm::scale(model, glm::vec3(sprite.GetWidth(), sprite.GetHeight(), 1.0f));
    
    // Set uniforms
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 
                       1, GL_FALSE, &camera->GetProjectionMatrix()[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 
                       1, GL_FALSE, &model[0][0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &color[0]);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 1);
    
    // Bind texture
    if (sprite.GetTexture()) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, sprite.GetTexture()->GetID());
    }
    
    // Draw
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::DrawRect(const Rect& rect, const glm::vec3& color) {
    glUseProgram(shaderProgram);
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(rect.x, rect.y, 0.0f));
    model = glm::scale(model, glm::vec3(rect.width, rect.height, 1.0f));
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 
                       1, GL_FALSE, &camera->GetProjectionMatrix()[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 
                       1, GL_FALSE, &model[0][0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &color[0]);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::DrawLine(const Vector2& start, const Vector2& end, 
                       const glm::vec3& color, float thickness) {
    // Simple line drawing implementation
    Vector2 dir = end - start;
    float length = dir.Length();
    float angle = atan2(dir.y, dir.x);
    
    glUseProgram(shaderProgram);
    
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, glm::vec3(start.x, start.y, 0.0f));
    model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::scale(model, glm::vec3(length, thickness, 1.0f));
    
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 
                       1, GL_FALSE, &camera->GetProjectionMatrix()[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 
                       1, GL_FALSE, &model[0][0]);
    glUniform3fv(glGetUniformLocation(shaderProgram, "color"), 1, &color[0]);
    glUniform1i(glGetUniformLocation(shaderProgram, "useTexture"), 0);
    
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void Renderer::DrawCircle(const Vector2& center, float radius, 
                         const glm::vec3& color) {
    // Draw circle using line segments
    const int segments = 32;
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i / segments * 2.0f * 3.14159f;
        float angle2 = (float)(i + 1) / segments * 2.0f * 3.14159f;
        
        Vector2 p1(center.x + cos(angle1) * radius, center.y + sin(angle1) * radius);
        Vector2 p2(center.x + cos(angle2) * radius, center.y + sin(angle2) * radius);
        
        DrawLine(p1, p2, color, 2.0f);
    }
}

Vector2 Renderer::WorldToScreen(const Vector2& worldPos) const {
    // Isometric conversion: (x, y) world to screen
    float screenX = (worldPos.x - worldPos.y);
    float screenY = (worldPos.x + worldPos.y) * 0.5f;
    return Vector2(screenX, screenY);
}

Vector2 Renderer::ScreenToWorld(const Vector2& screenPos) const {
    // Inverse isometric conversion
    float worldX = (screenPos.x + screenPos.y * 2.0f) * 0.5f;
    float worldY = (screenPos.y * 2.0f - screenPos.x) * 0.5f;
    return Vector2(worldX, worldY);
}

Texture* Renderer::LoadTexture(const std::string& path) {
    auto it = textures.find(path);
    if (it != textures.end()) {
        return it->second.get();
    }
    
    auto texture = std::make_unique<Texture>();
    if (texture->Load(path)) {
        Texture* ptr = texture.get();
        textures[path] = std::move(texture);
        return ptr;
    }
    
    return nullptr;
}

Texture* Renderer::GetTexture(const std::string& name) {
    auto it = textures.find(name);
    if (it != textures.end()) {
        return it->second.get();
    }
    return nullptr;
}

void Renderer::BeginBatch() {
    // For future optimization
}

void Renderer::EndBatch() {
    // For future optimization
}

void Renderer::InitializeQuad() {
    float vertices[] = {
        // positions   // texCoords
        0.0f, 1.0f,    0.0f, 1.0f,
        1.0f, 0.0f,    1.0f, 0.0f,
        0.0f, 0.0f,    0.0f, 0.0f,
        
        0.0f, 1.0f,    0.0f, 1.0f,
        1.0f, 1.0f,    1.0f, 1.0f,
        1.0f, 0.0f,    1.0f, 0.0f
    };
    
    glGenVertexArrays(1, &quadVAO);
    glGenBuffers(1, &quadVBO);
    
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    
    glBindVertexArray(quadVAO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 
                         (void*)(2 * sizeof(float)));
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void Renderer::InitializeShaders() {
    shaderProgram = CompileShader(vertexShaderSource, fragmentShaderSource);
}

unsigned int Renderer::CompileShader(const char* vertexSrc, const char* fragmentSrc) {
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexSrc, nullptr);
    glCompileShader(vertexShader);
    
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, nullptr, infoLog);
        std::cerr << "Vertex shader compilation failed:\n" << infoLog << std::endl;
    }
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentSrc, nullptr);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, nullptr, infoLog);
        std::cerr << "Fragment shader compilation failed:\n" << infoLog << std::endl;
    }
    
    unsigned int program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "Shader program linking failed:\n" << infoLog << std::endl;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return program;
}