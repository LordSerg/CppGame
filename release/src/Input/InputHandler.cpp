#include "InputHandler.h"
#include <cstring>
#include <algorithm>

InputHandler* InputHandler::instance = nullptr;

InputHandler::InputHandler(GLFWwindow* window)
    : window(window)
    , scrollDelta(0)
    , selecting(false)
    , mouseOverUI(false)
{
    std::memset(keyStates, 0, sizeof(keyStates));
    std::memset(prevKeyStates, 0, sizeof(prevKeyStates));
    std::memset(mouseButtonStates, 0, sizeof(mouseButtonStates));
    std::memset(prevMouseButtonStates, 0, sizeof(prevMouseButtonStates));
    
    instance = this;
    glfwSetScrollCallback(window, ScrollCallback);
}

void InputHandler::Update() {
    // Update previous states
    std::memcpy(prevKeyStates, keyStates, sizeof(keyStates));
    std::memcpy(prevMouseButtonStates, mouseButtonStates, sizeof(mouseButtonStates));
    
    // Update keyboard states
    for (int key = 0; key < GLFW_KEY_LAST; key++) {
        keyStates[key] = glfwGetKey(window, key) == GLFW_PRESS;
    }
    
    // Update mouse button states
    mouseButtonStates[0] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    mouseButtonStates[1] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
    mouseButtonStates[2] = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS;
    
    // Update mouse position
    prevMousePosition = mousePosition;
    double xpos, ypos;
    glfwGetCursorPos(window, &xpos, &ypos);
    mousePosition = Vector2(xpos, ypos);
    
    // Handle selection box start
    if (IsMouseButtonPressed(MouseButton::LEFT) && !mouseOverUI) {
        selecting = true;
        selectionStart = mousePosition;
    }
    // Note: selecting is NOT cleared here on button release.
    // The game code must call EndSelection() after reading the selection rect.
}

bool InputHandler::IsKeyPressed(int key) const {
    if (key < 0 || key >= GLFW_KEY_LAST) return false;
    return keyStates[key] && !prevKeyStates[key];
}

bool InputHandler::IsKeyDown(int key) const {
    if (key < 0 || key >= GLFW_KEY_LAST) return false;
    return keyStates[key];
}

bool InputHandler::IsKeyReleased(int key) const {
    if (key < 0 || key >= GLFW_KEY_LAST) return false;
    return !keyStates[key] && prevKeyStates[key];
}

bool InputHandler::IsMouseButtonPressed(MouseButton button) const {
    int idx = static_cast<int>(button);
    return mouseButtonStates[idx] && !prevMouseButtonStates[idx];
}

bool InputHandler::IsMouseButtonDown(MouseButton button) const {
    int idx = static_cast<int>(button);
    return mouseButtonStates[idx];
}

bool InputHandler::IsMouseButtonReleased(MouseButton button) const {
    int idx = static_cast<int>(button);
    return !mouseButtonStates[idx] && prevMouseButtonStates[idx];
}

Vector2 InputHandler::GetMousePosition() const {
    return mousePosition;
}

Vector2 InputHandler::GetMouseDelta() const {
    return mousePosition - prevMousePosition;
}

float InputHandler::GetScrollDelta() const {
    return scrollDelta;
}

Rect InputHandler::GetSelectionRect() const {
    if (!selecting) return Rect(0, 0, 0, 0);
    
    int x = std::min(selectionStart.x, mousePosition.x);
    int y = std::min(selectionStart.y, mousePosition.y);
    int w = std::abs(mousePosition.x - selectionStart.x);
    int h = std::abs(mousePosition.y - selectionStart.y);
    
    return Rect(x, y, w, h);
}

void InputHandler::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset) {
    if (instance) {
        instance->scrollDelta = yoffset;
    }
}