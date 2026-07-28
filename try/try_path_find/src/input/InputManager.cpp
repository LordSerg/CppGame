#include "input/InputManager.h"

float InputManager::accumulatedScroll = 0.0f;

InputManager::InputManager() {}

void InputManager::scrollCallback(GLFWwindow* window, double xoff, double yoff) {
    accumulatedScroll += (float)yoff;
}

void InputManager::processInput(GLFWwindow* window, float dt) {
    // Mouse position
    glfwGetCursorPos(window, &mouseX, &mouseY);

    // Left mouse button
    bool currentLeft = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    leftMousePressed = currentLeft && !prevLeftDown_;
    leftMouseReleased = !currentLeft && prevLeftDown_;
    leftMouseDown = currentLeft;
    prevLeftDown_ = currentLeft;

    // Right mouse button
    bool currentRight = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    rightMousePressed = currentRight && !prevRightDown_;
    prevRightDown_ = currentRight;

    // Keyboard camera movement
    cameraDX = 0;
    cameraDY = 0;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        cameraDY -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        cameraDY += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
        cameraDX -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS ||
        glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
        cameraDX += 1.0f;

    // Scroll
    scrollDelta = accumulatedScroll;
    accumulatedScroll = 0.0f;
}