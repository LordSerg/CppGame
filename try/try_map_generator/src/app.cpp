#include "app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <random>
#include <string>
#include <algorithm>

App::App()
    : window_(nullptr), windowWidth_(1280), windowHeight_(720),
      currentPattern_(0), currentSize_(0), currentPlacement_(0),
      numPlayers_(2),
      currentSeed_(12345),
      waterDensity_(1.0f), waterWidth_(1.0f),
      metalDensity_(1.0f), metalWidth_(1.0f),
      showUnderground_(false), showBarriers_(false),
      mapGenerated_(false),
      camOffsetX_(0), camOffsetY_(0), camZoom_(1.0f),
      isDragging_(false), lastMouseX_(0), lastMouseY_(0),
      camMoveSpeed_(400.0f) {
    snprintf(seedText_, sizeof(seedText_), "%u", currentSeed_);
}

App::~App() {
    shutdown();
}

void App::zoomAtScreenPoint(float screenX, float screenY, float zoomDelta) {
    float worldX = (screenX - camOffsetX_) / camZoom_;
    float worldY = (screenY - camOffsetY_) / camZoom_;

    camZoom_ *= zoomDelta;
    camZoom_ = std::clamp(camZoom_, 0.02f, 30.0f);

    camOffsetX_ = screenX - worldX * camZoom_;
    camOffsetY_ = screenY - worldY * camZoom_;
}

void App::scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    if (!app) return;
    if (ImGui::GetIO().WantCaptureMouse) return;

    float centerX = (float)app->windowWidth_ * 0.5f;
    float centerY = (float)app->windowHeight_ * 0.5f;

    float zoomFactor = (yoffset > 0) ? 1.12f : (1.0f / 1.12f);
    app->zoomAtScreenPoint(centerX, centerY, zoomFactor);
}

bool App::init(int width, int height, const char* title) {
    windowWidth_ = width;
    windowHeight_ = height;

    if (!glfwInit()) {
        fprintf(stderr, "Failed to init GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window_ = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!window_) {
        fprintf(stderr, "Failed to create GLFW window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);
    glfwSetWindowUserPointer(window_, this);
    glfwSetScrollCallback(window_, scrollCallback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        fprintf(stderr, "Failed to init GLAD\n");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    renderer_.init();

    return true;
}

void App::rerollSeed() {
    std::random_device rd;
    currentSeed_ = rd();
    snprintf(seedText_, sizeof(seedText_), "%u", currentSeed_);
}

void App::centerCameraOnMap() {
    if (!mapGenerated_) return;

    float mapW = (float)mapData_.getWidth();
    float mapH = (float)mapData_.getHeight();

    float scaleX = (float)(windowWidth_ - 20) / mapW;
    float scaleY = (float)(windowHeight_ - 20) / mapH;
    camZoom_ = std::min(scaleX, scaleY) * 0.9f;

    float renderedW = mapW * camZoom_;
    float renderedH = mapH * camZoom_;
    camOffsetX_ = ((float)windowWidth_ - renderedW) * 0.5f;
    camOffsetY_ = ((float)windowHeight_ - renderedH) * 0.5f;
}

void App::generateMap() {
    currentSeed_ = (uint32_t)strtoul(seedText_, nullptr, 10);

    GenerationParams params;
    switch (currentSize_) {
        case 0: params.size = MapSize::Small; break;
        case 1: params.size = MapSize::Mid; break;
        case 2: params.size = MapSize::Big; break;
        case 3: params.size = MapSize::Mammoth; break;
        default: params.size = MapSize::Small;
    }

    params.pattern = (MapPattern)currentPattern_;
    params.placement = (PlacementMode)currentPlacement_;
    params.numPlayers = numPlayers_;
    params.seed = currentSeed_;
    params.water.densityMultiplier = waterDensity_;
    params.water.widthMultiplier = waterWidth_;
    params.metal.densityMultiplier = metalDensity_;
    params.metal.widthMultiplier = metalWidth_;
    params.metal.intensityMultiplier = 1.0f;

    generator_.generate(mapData_, params);
    mapGenerated_ = true;
    renderer_.invalidateTextures();
    centerCameraOnMap();
}

void App::processInput(float deltaTime) {
    if (!ImGui::GetIO().WantCaptureKeyboard) {
        float pixelMove = camMoveSpeed_ * deltaTime;

        if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS) {
            camOffsetY_ += pixelMove;
        }
        if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS) {
            camOffsetY_ -= pixelMove;
        }
        if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS) {
            camOffsetX_ += pixelMove;
        }
        if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS) {
            camOffsetX_ -= pixelMove;
        }

        if (glfwGetKey(window_, GLFW_KEY_EQUAL) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS) {
            float cx = (float)windowWidth_ * 0.5f;
            float cy = (float)windowHeight_ * 0.5f;
            zoomAtScreenPoint(cx, cy, 1.0f + 1.5f * deltaTime);
        }
        if (glfwGetKey(window_, GLFW_KEY_MINUS) == GLFW_PRESS ||
            glfwGetKey(window_, GLFW_KEY_Q) == GLFW_PRESS) {
            float cx = (float)windowWidth_ * 0.5f;
            float cy = (float)windowHeight_ * 0.5f;
            zoomAtScreenPoint(cx, cy, 1.0f - 1.5f * deltaTime);
        }

        if (glfwGetKey(window_, GLFW_KEY_HOME) == GLFW_PRESS) {
            centerCameraOnMap();
        }
    }

    if (ImGui::GetIO().WantCaptureMouse) {
        isDragging_ = false;
        return;
    }

    double mx, my;
    glfwGetCursorPos(window_, &mx, &my);

    if (glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ||
        glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS) {
        if (isDragging_) {
            double dx = mx - lastMouseX_;
            double dy = my - lastMouseY_;
            camOffsetX_ += (float)dx;
            camOffsetY_ += (float)dy;
        }
        isDragging_ = true;
    } else {
        isDragging_ = false;
    }

    lastMouseX_ = mx;
    lastMouseY_ = my;
}

void App::renderUI() {
    ImGui::Begin("Map Generator Controls", nullptr, ImGuiWindowFlags_AlwaysAutoResize);

    // === Pattern ===
    const char* patterns[] = {"Cell", "Star", "Archipelago"};
    ImGui::Combo("Map Pattern", &currentPattern_, patterns, IM_ARRAYSIZE(patterns));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.7f, 0.7f, 0.7f, 1.0f));
    switch (currentPattern_) {
        case 0:
            ImGui::TextWrapped("Players enclosed in rock walls. Break out to access common forest.");
            break;
        case 1:
            ImGui::TextWrapped("Players on edges, paths radiate to center through forest.");
            break;
        case 2:
            ImGui::TextWrapped("Noise-based water terrain with land bridges.");
            break;
    }
    ImGui::PopStyleColor();

    ImGui::Separator();

    // === Placement mode ===
    const char* placements[] = {
        "Circle (Symmetric)",
        "Organic (Natural shapes)",
        "Voronoi (Territory cells)",
        "Spiral (Golden ratio)",
        "Clustered (Teams)"
    };
    ImGui::Combo("Placement", &currentPlacement_, placements, IM_ARRAYSIZE(placements));

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.65f, 0.75f, 0.65f, 1.0f));
    switch (currentPlacement_) {
        case 0:
            ImGui::TextWrapped("Players equally spaced on a circle. Classic symmetric layout.");
            break;
        case 1:
            ImGui::TextWrapped("Positions jittered, areas have noise-distorted organic boundaries like natural formations.");
            break;
        case 2:
            ImGui::TextWrapped("Areas defined by Voronoi cells with noise-warped edges. Territory-like shapes.");
            break;
        case 3:
            ImGui::TextWrapped("Golden-angle spiral placement (Fibonacci). Organic shapes. Unique asymmetric feel.");
            break;
        case 4:
            ImGui::TextWrapped("Players grouped into 2-4 clusters (team positions). Organic shapes within clusters.");
            break;
    }
    ImGui::PopStyleColor();

    ImGui::Separator();

    // === Size ===
    const char* sizes[] = {"Small (500x500)", "Mid (1000x1000)", "Big (2000x2000)", "Mammoth (4000x4000)"};
    ImGui::Combo("Map Size", &currentSize_, sizes, IM_ARRAYSIZE(sizes));

    // === Players ===
    ImGui::SliderInt("Players", &numPlayers_, 2, 8);

    // === Seed ===
    ImGui::Separator();
    ImGui::Text("Seed:");
    ImGui::PushItemWidth(150);
    ImGui::InputText("##seed", seedText_, sizeof(seedText_), ImGuiInputTextFlags_CharsDecimal);
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Reroll")) {
        rerollSeed();
    }

    // === Water ===
    ImGui::Separator();
    ImGui::Text("Water (Rivers):");
    ImGui::SliderFloat("River Density", &waterDensity_, 0.2f, 4.0f, "%.1f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Number of river sources and branching frequency");
    ImGui::SliderFloat("River Width", &waterWidth_, 0.3f, 4.0f, "%.1f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Thickness of river channels");

    // === Metal ===
    ImGui::Separator();
    ImGui::Text("Metal (Underground Veins):");
    ImGui::SliderFloat("Vein Density", &metalDensity_, 0.3f, 4.0f, "%.1f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Number of metal vein clusters");
    ImGui::SliderFloat("Vein Width", &metalWidth_, 0.3f, 4.0f, "%.1f");
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Thickness of underground metal veins");

    // === Generate ===
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.6f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.7f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.15f, 0.5f, 0.15f, 1.0f));
    if (ImGui::Button("Generate Map", ImVec2(220, 45))) {
        generateMap();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();
    if (ImGui::Button("Quick\nReroll", ImVec2(70, 45))) {
        rerollSeed();
        generateMap();
    }
    if (ImGui::IsItemHovered())
        ImGui::SetTooltip("Reroll seed and regenerate immediately");

    // === View ===
    ImGui::Separator();
    ImGui::Text("View Options:");
    if (ImGui::Checkbox("Show Underground (Metal Veins)", &showUnderground_)) {}
    if (ImGui::Checkbox("Show Barriers (Blocked Tiles)", &showBarriers_)) {
        renderer_.invalidateTextures();
    }

    // === Camera ===
    ImGui::Separator();
    ImGui::Text("Camera: Zoom %.2fx", camZoom_);
    ImGui::SliderFloat("Move Speed", &camMoveSpeed_, 100.0f, 2000.0f, "%.0f px/s");
    if (ImGui::Button("Reset View")) {
        centerCameraOnMap();
    }

    // === Navigation help ===
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
    ImGui::Text("Navigation:");
    ImGui::BulletText("WASD / Arrows: Pan");
    ImGui::BulletText("Q/E or +/-: Zoom");
    ImGui::BulletText("Mouse drag: Pan");
    ImGui::BulletText("Scroll: Zoom to center");
    ImGui::BulletText("Home: Reset view");
    ImGui::PopStyleColor();

    // === Info ===
    if (mapGenerated_) {
        ImGui::Separator();
        ImGui::Text("Map: %dx%d  |  Players: %d  |  Seed: %u",
                     mapData_.getWidth(), mapData_.getHeight(),
                     (int)mapData_.getStartingAreas().size(), currentSeed_);
        ImGui::Text("Pattern: %s  |  Placement: %s",
                     patterns[currentPattern_], placements[currentPlacement_]);

        // Starting area info
        if (ImGui::TreeNode("Starting Areas")) {
            for (auto& sa : mapData_.getStartingAreas()) {
                ImGui::Text("Player %d: (%d, %d) r=%d %s",
                            sa.playerIndex + 1, sa.centerX, sa.centerY, sa.radius,
                            sa.hasShape() ? "(shaped)" : "(circle)");
            }
            ImGui::TreePop();
        }

        ImGui::Separator();
        ImGui::Text("Legend:");
        ImGui::TextColored(ImVec4(0.55f, 0.35f, 0.17f, 1.0f), "  Brown = Ground");
        ImGui::TextColored(ImVec4(0.13f, 0.55f, 0.13f, 1.0f), "  Green = Trees (removable)");
        ImGui::TextColored(ImVec4(0.59f, 0.59f, 0.59f, 1.0f), "  Gray = Rocks (removable)");
        ImGui::TextColored(ImVec4(0.12f, 0.39f, 0.78f, 1.0f), "  Blue = Water (permanent)");
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.1f, 1.0f),    "  Orange = Metal (underground)");
        ImGui::Text("  Colored dots = Player starts");
    }

    ImGui::End();
}

void App::run() {
    double lastTime = glfwGetTime();

    while (!glfwWindowShouldClose(window_)) {
        double currentTime = glfwGetTime();
        float deltaTime = (float)(currentTime - lastTime);
        lastTime = currentTime;
        deltaTime = std::min(deltaTime, 0.05f);

        glfwPollEvents();
        processInput(deltaTime);

        glfwGetFramebufferSize(window_, &windowWidth_, &windowHeight_);
        glViewport(0, 0, windowWidth_, windowHeight_);
        glClearColor(0.12f, 0.12f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        if (mapGenerated_) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            renderer_.renderMap(mapData_, camOffsetX_, camOffsetY_, camZoom_,
                                showUnderground_, showBarriers_,
                                (float)windowWidth_, (float)windowHeight_);
            glDisable(GL_BLEND);
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        renderUI();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window_);
    }
}

void App::shutdown() {
    renderer_.shutdown();

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    if (window_) {
        glfwDestroyWindow(window_);
        window_ = nullptr;
    }
    glfwTerminate();
}