#pragma once

#include "simulation/World.h"
#include "rendering/Renderer.h"
#include "camera/Camera.h"
#include "input/InputManager.h"
#include "pathfinding/IPathfinder.h"
#include "pathfinding/Grid.h"
#include "pathfinding/NavMesh.h"
#include "steering/ISteeringBehavior.h"
#include "formation/IFormation.h"

#include <GLFW/glfw3.h>
#include <memory>
#include <vector>
#include <string>

class Application {
public:
    Application();
    ~Application();

    bool init();
    void run();
    void shutdown();

private:
    void update(float dt);
    void render();
    void renderUI();
    void handleInput(float dt);
    void notifyWorldChanged();

    GLFWwindow* window_ = nullptr;
    World world_;
    Renderer renderer_;
    Camera camera_;
    InputManager input_;

    // Algorithm choices
    int currentPathfinder_ = 0;
    int currentSteering_ = 0;
    int currentFormation_ = 0;
    int currentMapTool_ = 0; // 0=none, 1=barrier, 2=agent

    // Visualization toggles
    bool showPaths_ = true;
    bool showGrid_ = false;
    bool showNavMesh_ = false;

    // Algorithm instances
    std::vector<std::shared_ptr<IPathfinder>> pathfinders_;
    std::vector<std::shared_ptr<ISteeringBehavior>> steerings_;
    std::vector<std::shared_ptr<IFormation>> formations_;

    // For grid/navmesh visualization
    Grid visualGrid_;
    NavMesh navMesh_;
    bool gridDirty_ = true;

    // Simulation state
    bool paused_ = false;
};