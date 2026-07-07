#ifndef INPUTHANDLER_H
#define INPUTHANDLER_H

#include "../Utils/Math.h"
#include <cstring>
#include <GLFW/glfw3.h>

enum class MouseButton {
    LEFT,
    RIGHT,
    MIDDLE
};

class InputHandler {
public:
    InputHandler(GLFWwindow* window);
    
    void Update();
    
    // Keyboard
    bool IsKeyPressed(int key) const;
    bool IsKeyDown(int key) const;
    bool IsKeyReleased(int key) const;
    
    // Mouse
    bool IsMouseButtonPressed(MouseButton button) const;
    bool IsMouseButtonDown(MouseButton button) const;
    bool IsMouseButtonReleased(MouseButton button) const;
    
    Vector2 GetMousePosition() const;
    Vector2 GetMouseDelta() const;
    float GetScrollDelta() const;
    
    // Selection box
    bool IsSelecting() const { return selecting; }
    Rect GetSelectionRect() const;
    void EndSelection() { selecting = false; }
    
    // Reset all input state (call when transitioning to a new game state)
    void ResetState() {
        selecting = false;
        scrollDelta = 0;
        std::memset(keyStates, 0, sizeof(keyStates));
        std::memset(prevKeyStates, 0, sizeof(prevKeyStates));
        std::memset(mouseButtonStates, 0, sizeof(mouseButtonStates));
        std::memset(prevMouseButtonStates, 0, sizeof(prevMouseButtonStates));
    }
    
    bool IsMouseOverUI() const { return mouseOverUI; }
    void SetMouseOverUI(bool value) { mouseOverUI = value; }
    
    // Scroll
    void ResetScrollDelta() { scrollDelta = 0; }
    
private:
    GLFWwindow* window;
    
    bool keyStates[GLFW_KEY_LAST];
    bool prevKeyStates[GLFW_KEY_LAST];
    
    bool mouseButtonStates[3];
    bool prevMouseButtonStates[3];
    
    Vector2 mousePosition;
    Vector2 prevMousePosition;
    float scrollDelta;
    
    bool selecting;
    Vector2 selectionStart;
    bool mouseOverUI;
    
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static InputHandler* instance;
};

#endif // INPUTHANDLER_H