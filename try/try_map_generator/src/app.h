#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "map_data.h"
#include "map_generator.h"
#include "renderer.h"

class App {
public:
    App();
    ~App();

    bool init(int width, int height, const char* title);
    void run();
    void shutdown();

private:
    GLFWwindow* window_;
    int windowWidth_, windowHeight_;

    MapData mapData_;
    MapGenerator generator_;
    Renderer renderer_;

    // UI State
    int currentPattern_;
    int currentSize_;
    int numPlayers_;
    char seedText_[32];
    uint32_t currentSeed_;
    bool showUnderground_;
    bool showBarriers_;
    bool mapGenerated_;

    // Camera
    float camOffsetX_, camOffsetY_;
    float camZoom_;
    bool isDragging_;
    double lastMouseX_, lastMouseY_;

    void renderUI();
    void processInput();
    void rerollSeed();

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};