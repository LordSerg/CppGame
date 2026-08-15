#include "app.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <random>
#include <string>

App::App()
    : window_(nullptr), windowWidth_(1280), windowHeight_(720),
      currentPattern_(0), currentSize_(0), numPlayers_(2),
      currentSeed_(12345), showUnderground_(false), showBarriers_(false),
      mapGenerated_(false),
      camOffsetX_(0), camOffsetY_(0), camZoom_(1.0f),
      isDragging_(false), lastMouseX_(0), lastMouseY_(0) {
    snprintf(seedText_, sizeof(seedText_), "%u", currentSeed_);
}

App::~App() {
    shutdown();
}

void App::scrollCallback(GLFWwindow* window, double /*xoffset*/, double yoffset) {
    App* app = (App*)glfwGetWindowUserPointer(window);
    if (!app) return;

    // Don't zoom if ImGui wants the mouse
    if (ImGui::GetIO().WantCaptureMouse) return;

    float zoomFactor = 1.1f;
    if (yoffset > 0) {
        app->camZoom_ *= zoomFactor;
    } else {
        app->camZoom_ /= zoomFactor;
    }
    app->camZoom_ = std::max(0.05f, std::min(app->camZoom_, 20.0f));
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

    // Init ImGui
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

void App::processInput() {
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

    // Pattern selection
    const char* patterns[] = {"Cell", "Star", "Archipelago"};
    ImGui::Combo("Map Pattern", &currentPattern_, patterns, IM_ARRAYSIZE(patterns));

    // Size selection
    const char* sizes[] = {"Small (500x500)", "Mid (1000x1000)", "Big (2000x2000)", "Mammoth (4000x4000)"};
    ImGui::Combo("Map Size", &currentSize_, sizes, IM_ARRAYSIZE(sizes));

    // Player count
    ImGui::SliderInt("Players", &numPlayers_, 2, 8);

    // Seed
    ImGui::Separator();
    ImGui::Text("Seed:");
    ImGui::PushItemWidth(150);
    ImGui::InputText("##seed", seedText_, sizeof(seedText_));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Reroll")) {
        rerollSeed();
    }

    // Generate button
    ImGui::Separator();
    if (ImGui::Button("Generate", ImVec2(200, 40))) {
        currentSeed_ = (uint32_t)strtoul(seedText_, nullptr, 10);

        MapSize mapSize;
        switch (currentSize_) {
            case 0: mapSize = MapSize::Small; break;
            case 1: mapSize = MapSize::Mid; break;
            case 2: mapSize = MapSize::Big; break;
            case 3: mapSize = MapSize::Mammoth; break;
            default: mapSize = MapSize::Small;
        }

        MapPattern pattern = (MapPattern)currentPattern_;
        generator_.generate(mapData_, mapSize, pattern, numPlayers_, currentSeed_);
        mapGenerated_ = true;

        // Reset camera to fit map
        camZoom_ = std::min(
            (float)(windowWidth_ - 250) / mapData_.getWidth(),
            (float)windowHeight_ / mapData_.getHeight()
        ) * 0.9f;
        camOffsetX_ = 120.0f;
        camOffsetY_ = 10.0f;
    }

    // View controls
    ImGui::Separator();
    ImGui::Text("View Options:");
    ImGui::Checkbox("Show Underground (Metal)", &showUnderground_);
    ImGui::Checkbox("Show Barriers", &showBarriers_);

    // Camera controls
    ImGui::Separator();
    ImGui::Text("Camera:");
    ImGui::Text("Zoom: %.2fx", camZoom_);
    if (ImGui::Button("Reset View")) {
        if (mapGenerated_) {
            camZoom_ = std::min(
                (float)(windowWidth_ - 250) / mapData_.getWidth(),
                (float)windowHeight_ / mapData_.getHeight()
            ) * 0.9f;
            camOffsetX_ = 120.0f;
            camOffsetY_ = 10.0f;
        }
    }
    ImGui::Text("Drag: LMB/MMB | Zoom: Scroll");

    // Info
    if (mapGenerated_) {
        ImGui::Separator();
        ImGui::Text("Map: %dx%d", mapData_.getWidth(), mapData_.getHeight());
        ImGui::Text("Starting areas: %d", (int)mapData_.getStartingAreas().size());

        // Legend
        ImGui::Separator();
        ImGui::Text("Legend:");
        ImGui::TextColored(ImVec4(0.55f, 0.35f, 0.17f, 1.0f), "  Brown = Ground");
        ImGui::TextColored(ImVec4(0.13f, 0.55f, 0.13f, 1.0f), "  Green = Trees");
        ImGui::TextColored(ImVec4(0.59f, 0.59f, 0.59f, 1.0f), "  Gray = Rocks");
        ImGui::TextColored(ImVec4(0.12f, 0.39f, 0.78f, 1.0f), "  Blue = Water");
        ImGui::TextColored(ImVec4(0.8f, 0.5f, 0.1f, 1.0f),    "  Orange = Metal (underground)");
        ImGui::Text("  Colored dots = Player starts");
    }

    ImGui::End();
}

void App::run() {
    while (!glfwWindowShouldClose(window_)) {
        glfwPollEvents();
        processInput();

        glfwGetFramebufferSize(window_, &windowWidth_, &windowHeight_);
        glViewport(0, 0, windowWidth_, windowHeight_);
        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // Render map
        if (mapGenerated_) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            renderer_.renderMap(mapData_, camOffsetX_, camOffsetY_, camZoom_,
                                showUnderground_, showBarriers_,
                                (float)windowWidth_, (float)windowHeight_);
            glDisable(GL_BLEND);
        }

        // Render ImGui
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