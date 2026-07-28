#pragma once

#include "simulation/Agent.h"
#include <GLFW/glfw3.h>

enum class MapTool {
    None,        // Selection
    AddBarrier,
    AddAgent
};

class InputManager {
public:
    InputManager();

    void processInput(GLFWwindow* window, float dt);

    // Mouse state
    bool leftMouseDown = false;
    bool leftMousePressed = false;  // just pressed this frame
    bool leftMouseReleased = false; // just released this frame
    bool rightMousePressed = false;
    double mouseX = 0, mouseY = 0;

    // Selection drag
    bool isDragging = false;
    Vec2 dragStart;
    Vec2 dragEnd;

    // Camera movement
    float cameraDX = 0, cameraDY = 0;

    // Scroll
    float scrollDelta = 0;

    // Current tool
    MapTool currentTool = MapTool::None;

    // Static scroll callback
    static void scrollCallback(GLFWwindow* window, double xoff, double yoff);
    static float accumulatedScroll;

private:
    bool prevLeftDown_ = false;
    bool prevRightDown_ = false;
};