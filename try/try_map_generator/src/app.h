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

    // Generation params exposed to UI
    int currentPattern_;
    int currentSize_;
    int numPlayers_;
    char seedText_[32];
    uint32_t currentSeed_;

    // Water controls
    float waterDensity_;     // 0.5 - 3.0, default 1.0
    float waterWidth_;       // 0.5 - 3.0, default 1.0

    // Metal controls
    float metalDensity_;     // 0.5 - 3.0, default 1.0
    float metalWidth_;       // 0.5 - 3.0, default 1.0

    // View toggles
    bool showUnderground_;
    bool showBarriers_;
    bool mapGenerated_;

    // Camera
    float camOffsetX_, camOffsetY_;
    float camZoom_;
    bool isDragging_;
    double lastMouseX_, lastMouseY_;

    // Keyboard move speed
    float camMoveSpeed_;

    void renderUI();
    void processInput(float deltaTime);
    void rerollSeed();
    void centerCameraOnMap();

    // Zoom toward a screen point (for center-of-window zoom)
    void zoomAtScreenPoint(float screenX, float screenY, float zoomDelta);

    static void scrollCallback(GLFWwindow* window, double xoffset, double yoffset);
};