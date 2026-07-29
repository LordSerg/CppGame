#include "core/Application.h"
#include "core/Config.h"

// Pathfinders
#include "pathfinding/AStarPathfinder.h"
#include "pathfinding/DijkstraPathfinder.h"
#include "pathfinding/ThetaStarPathfinder.h"
#include "pathfinding/FlowFieldPathfinder.h"

// Steering
#include "steering/SeekBehavior.h"
#include "steering/SeparationBehavior.h"
#include "steering/CollisionAvoidance.h"
#include "steering/ORCABehavior.h"
#include "steering/FlockingBehavior.h"

// Formation
#include "formation/SquareFormation.h"
#include "formation/CircleFormation.h"
#include "formation/LineFormation.h"
#include "formation/WedgeFormation.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <algorithm>
#include <cmath>

Application::Application() {}

Application::~Application() {
    shutdown();
}

bool Application::init() {
    if (!glfwInit()) return false;

    // OpenGL hints for compatibility
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

    window_ = glfwCreateWindow(Config::WINDOW_WIDTH, Config::WINDOW_HEIGHT,
                                "Walking Simulation", nullptr, nullptr);
    if (!window_) {
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1); // vsync

    glfwSetScrollCallback(window_, InputManager::scrollCallback);

    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 120");

    // Initialize algorithm instances
    pathfinders_.push_back(std::make_shared<AStarPathfinder>());
    pathfinders_.push_back(std::make_shared<DijkstraPathfinder>());
    pathfinders_.push_back(std::make_shared<ThetaStarPathfinder>());
    pathfinders_.push_back(std::make_shared<FlowFieldPathfinder>());

    steerings_.push_back(std::make_shared<SeekBehavior>());
    steerings_.push_back(std::make_shared<SeparationBehavior>());
    steerings_.push_back(std::make_shared<CollisionAvoidance>());
    steerings_.push_back(std::make_shared<ORCABehavior>());
    steerings_.push_back(std::make_shared<FlockingBehavior>());

    formations_.push_back(std::make_shared<SquareFormation>());
    formations_.push_back(std::make_shared<CircleFormation>());
    formations_.push_back(std::make_shared<LineFormation>());
    formations_.push_back(std::make_shared<WedgeFormation>());

    // Set defaults
    world_.setPathfinder(pathfinders_[0]);
    world_.setSteeringBehavior(steerings_[0]);
    world_.setFormation(formations_[0]);

    return true;
}

void Application::notifyWorldChanged() {
    gridDirty_ = true;
    if (world_.getPathfinder())
        world_.getPathfinder()->onWorldChanged(world_);
}

void Application::handleInput(float dt) {
    ImGuiIO& io = ImGui::GetIO();
    input_.processInput(window_, dt);

    // Camera movement
    if (!io.WantCaptureKeyboard) {
        camera_.pan(input_.cameraDX * camera_.panSpeed * dt / camera_.zoomLevel,
                    input_.cameraDY * camera_.panSpeed * dt / camera_.zoomLevel);
    }

    // Scroll zoom
    if (!io.WantCaptureMouse && input_.scrollDelta != 0.0f) {
        float factor = (input_.scrollDelta > 0) ? 1.1f : 0.9f;
        camera_.zoom(factor);
    }

    // Determine viewport (excluding UI panel)
    int winW, winH;
    glfwGetWindowSize(window_, &winW, &winH);
    float viewportW = winW - Config::UI_PANEL_WIDTH;
    float viewportH = (float)winH;

    // Only handle map clicks if mouse is in viewport area and imgui doesn't want it
    bool mouseInViewport = (input_.mouseX < viewportW) && !io.WantCaptureMouse;

    // Convert mouse to world coords
    Vec2 worldMouse = camera_.screenToWorld((float)input_.mouseX, (float)input_.mouseY,
                                             viewportW, viewportH);

    // Tool handling
    MapTool tool = MapTool::None;
    if (currentMapTool_ == 1) tool = MapTool::AddBarrier;
    else if (currentMapTool_ == 2) tool = MapTool::AddAgent;

    if (mouseInViewport) {
        // Right click - command selected agents
        if (input_.rightMousePressed) {
            world_.commandSelectedTo(worldMouse);
        }

        // Left mouse actions
        if (tool == MapTool::AddAgent) {
            if (input_.leftMouseDown) {
                // Add agent at mouse position (rate limited by checking distance to existing)
                bool tooClose = false;
                for (const auto& a : world_.getAgents()) {
                    if ((a.position - worldMouse).lengthSq() < 
                        Config::DEFAULT_AGENT_RADIUS * Config::DEFAULT_AGENT_RADIUS * 4.0f) {
                        tooClose = true;
                        break;
                    }
                }
                if (!tooClose) {
                    world_.addAgent(worldMouse);
                }
            }
        } else if (tool == MapTool::AddBarrier) {
            if (input_.leftMousePressed) {
                Vec2 halfExt(Config::DEFAULT_BARRIER_HALF_SIZE, Config::DEFAULT_BARRIER_HALF_SIZE);
                world_.addBarrier(worldMouse, halfExt);
                notifyWorldChanged();
            }
        } else {
            // Selection tool
            if (input_.leftMousePressed) {
                input_.isDragging = true;
                input_.dragStart = worldMouse;
                input_.dragEnd = worldMouse;
            }
            if (input_.leftMouseDown && input_.isDragging) {
                input_.dragEnd = worldMouse;
            }
            if (input_.leftMouseReleased && input_.isDragging) {
                input_.isDragging = false;
                Vec2 selMin(std::min(input_.dragStart.x, input_.dragEnd.x),
                            std::min(input_.dragStart.y, input_.dragEnd.y));
                Vec2 selMax(std::max(input_.dragStart.x, input_.dragEnd.x),
                            std::max(input_.dragStart.y, input_.dragEnd.y));

                // If it's basically a click (small rect), try to select single agent
                float selW = selMax.x - selMin.x;
                float selH = selMax.y - selMin.y;
                if (selW < 3.0f && selH < 3.0f) {
                    // Click select
                    world_.clearSelection();
                    for (auto& a : world_.getAgents()) {
                        if ((a.position - worldMouse).length() < a.radius + 3.0f) {
                            a.selected = true;
                            break;
                        }
                    }
                } else {
                    world_.selectAgentsInRect(selMin, selMax);
                }
            }
        }
    }

    // Delete key to remove selected agents/barriers
    if (!io.WantCaptureKeyboard && glfwGetKey(window_, GLFW_KEY_DELETE) == GLFW_PRESS) {
        auto selected = world_.getSelectedAgents();
        for (auto* a : selected) {
            world_.removeAgent(a->id);
        }
    }

    // Space to pause
    static bool spaceWasPressed = false;
    bool spaceDown = glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
    if (spaceDown && !spaceWasPressed) paused_ = !paused_;
    spaceWasPressed = spaceDown;
}

void Application::update(float dt) {
    if (!paused_) {
        world_.update(dt);
    }
}

void Application::render() {
    int winW, winH;
    glfwGetWindowSize(window_, &winW, &winH);
    float viewportW = winW - Config::UI_PANEL_WIDTH;
    float viewportH = (float)winH;

    // Set viewport to map area (left side)
    glViewport(0, 0, (int)viewportW, (int)viewportH);

    renderer_.beginFrame(viewportW, viewportH, camera_);

    // Draw world background
    renderer_.drawWorld(Config::MAP_WIDTH, Config::MAP_HEIGHT);

    // Grid visualization
    if (showGrid_) {
        if (gridDirty_) {
            visualGrid_.rebuild(world_);
            gridDirty_ = false;
        }
        renderer_.drawGrid(visualGrid_);
    }

    // NavMesh visualization
    if (showNavMesh_) {
        navMesh_.rebuild(world_);
        renderer_.drawNavMesh(navMesh_);
    }

    // Barriers
    renderer_.drawBarriers(world_.getBarriers());

    // Paths
    if (showPaths_) {
        renderer_.drawPaths(world_.getAgents());
    }

    // Agents
    renderer_.drawAgents(world_.getAgents());

    // Selection rectangle
    if (input_.isDragging && currentMapTool_ == 0) {
        renderer_.drawSelectionRect(input_.dragStart, input_.dragEnd);
    }

    renderer_.endFrame();
}

void Application::renderUI() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    int winW, winH;
    glfwGetWindowSize(window_, &winW, &winH);

    ImGui::SetNextWindowPos(ImVec2((float)winW - Config::UI_PANEL_WIDTH, 0));
    ImGui::SetNextWindowSize(ImVec2(Config::UI_PANEL_WIDTH, (float)winH));

    ImGui::Begin("Controls", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoCollapse);

    auto DrawCombo = [](const char* label, int& current, const auto& items) -> bool {
        std::vector<std::string> strings;
        std::vector<const char*> cstrs;
        for (const auto& item : items) strings.push_back(item->name());
        for (const auto& s : strings) cstrs.push_back(s.c_str());
        return ImGui::Combo(label, &current, cstrs.data(), (int)cstrs.size());
    };

    // --- Pathfinding ---
    ImGui::SeparatorText("Pathfinding Algorithm");
    if (DrawCombo("##pathfinder", currentPathfinder_, pathfinders_)) {
        world_.setPathfinder(pathfinders_[currentPathfinder_]);
    }

    // --- Steering ---
    ImGui::SeparatorText("Agent Interaction");
    if (DrawCombo("##steering", currentSteering_, steerings_)) {
        world_.setSteeringBehavior(steerings_[currentSteering_]);
    }

    // --- Formation ---
    ImGui::SeparatorText("Agent Formation");
    if (DrawCombo("##formation", currentFormation_, formations_)) {
        world_.setFormation(formations_[currentFormation_]);
    }

    // --- Map Tool ---
    ImGui::SeparatorText("Add to Map");
    ImGui::RadioButton("Nothing (Select)", &currentMapTool_, 0);
    ImGui::RadioButton("Add Barrier", &currentMapTool_, 1);
    ImGui::RadioButton("Add Agent", &currentMapTool_, 2);

    // --- Visualization ---
    ImGui::SeparatorText("Visualization");
    ImGui::Checkbox("Show Paths", &showPaths_);
    ImGui::Checkbox("Show Grid", &showGrid_);
    ImGui::Checkbox("Show NavMesh", &showNavMesh_);

    // --- Info ---
    ImGui::SeparatorText("Info");
    ImGui::Text("Agents: %d", (int)world_.getAgents().size());
    ImGui::Text("Barriers: %d", (int)world_.getBarriers().size());
    ImGui::Text("Selected: %d", (int)world_.getSelectedAgents().size());
    int pending = world_.pendingPathRequests();
    if (pending > 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
                           "Paths queued: %d", pending);
    }
    ImGui::Text("%s", paused_ ? "PAUSED (Space)" : "Running (Space to pause)");

    // --- Actions ---
    ImGui::SeparatorText("Actions");
    if (ImGui::Button("Clear All Agents")) world_.getAgents().clear();
    if (ImGui::Button("Clear All Barriers")) {
        world_.getBarriers().clear();
        notifyWorldChanged();
    }
    if (ImGui::Button("Clear Selection")) world_.clearSelection();
    if (ImGui::Button("Delete Selected (Del)")) {
        auto selected = world_.getSelectedAgents();
        for (auto* a : selected) world_.removeAgent(a->id);
    }

    // --- Help ---
    ImGui::SeparatorText("Controls");
    ImGui::TextWrapped(
        "LMB: Select/Add\n"
        "RMB: Move selected agents\n"
        "Scroll: Zoom\n"
        "WASD/Arrows: Pan camera\n"
        "Space: Pause/Resume\n"
        "Delete: Remove selected"
    );

    ImGui::End();
    ImGui::Render();
    glViewport(0, 0, winW, winH);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::run() {
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();

        double currentTime = glfwGetTime();
        float dt = (float)(currentTime - lastTime);
        lastTime = currentTime;

        // Cap delta time
        if (dt > 0.1f) dt = 0.1f;

        handleInput(dt);
        update(dt);
        render();
        renderUI();

        glfwSwapBuffers(window_);
    }
}

void Application::shutdown() {
    if (window_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        glfwDestroyWindow(window_);
        glfwTerminate();
        window_ = nullptr;
    }
}