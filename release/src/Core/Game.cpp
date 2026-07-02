#include "Game.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>

Game::Game() 
    : window(nullptr)
    , isRunning(false)
    , gameSpeed(1.0f)
    , screenWidth(1920)
    , screenHeight(1080)
    , humanPlayerId(0)
{
}

Game::~Game() {
}

bool Game::Initialize() {
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    window = glfwCreateWindow(screenWidth, screenHeight, "Warcraft 2 Clone", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Initialize subsystems
    renderer = std::make_unique<Renderer>();
    if (!renderer->Initialize(screenWidth, screenHeight)) {
        return false;
    }
    
    audioManager = std::make_unique<AudioManager>();
    if (!audioManager->Initialize()) {
        std::cerr << "Warning: Failed to initialize audio" << std::endl;
    }
    
    inputHandler = std::make_unique<InputHandler>(window);
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    menuSystem = std::make_unique<MenuSystem>();
    saveSystem = std::make_unique<SaveSystem>();
    
    // Start background music
    audioManager->PlayMusic("assets/audio/music/main_theme.ogg", true);
    
    isRunning = true;
    return true;
}

void Game::Run() {
    const float targetFPS = 60.0f;
    const float targetFrameTime = 1.0f / targetFPS;
    
    float lastTime = glfwGetTime();
    float accumulator = 0.0f;
    
    while (isRunning && !glfwWindowShouldClose(window)) {
        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        accumulator += deltaTime;
        
        // Fixed timestep updates
        while (accumulator >= targetFrameTime) {
            ProcessInput();
            Update(targetFrameTime * gameSpeed);
            accumulator -= targetFrameTime;
        }
        
        Render();
        
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

void Game::ProcessInput() {
    inputHandler->Update();
    
    if (CurrentGameState::CGS == GameState::PLAYING && !inputHandler->IsMouseOverUI()) {
        // Handle game input
        // Unit selection, commands, etc.
    }
}

void Game::Update(float deltaTime) {
    switch (CurrentGameState::CGS) {
        case GameState::MAIN_MENU:
            menuSystem->ShowMainMenu();
            menuSystem->Render(renderer.get());
        case GameState::NEW_GAME_MENU:
        case GameState::LOAD_GAME_MENU:
            menuSystem->Update(deltaTime);
            break;
            
        case GameState::PLAYING:
            if (map) {
                map->Update(deltaTime);
            }
            
            if (commandSystem) {
                commandSystem->Update(deltaTime);
            }
            
            if (combatSystem) {
                combatSystem->Update(deltaTime);
            }
            
            // Update AI
            for (auto& ai : aiControllers) {
                ai->Update(deltaTime, map.get(), resourceManager.get(), 
                          techTree.get(), commandSystem.get());
            }
            
            CheckWinCondition();
            break;
            
        case GameState::PAUSED:
            // Don't update game logic
            menuSystem->Update(deltaTime);
            break;
            
        default:
            break;
    }
    
    HandleStateTransitions();
}

void Game::Render() {
    renderer->Clear();
    
    switch (CurrentGameState::CGS) {
        case GameState::MAIN_MENU:
            menuSystem->ShowMainMenu();
            menuSystem->Render(renderer.get());
        case GameState::NEW_GAME_MENU:
        case GameState::LOAD_GAME_MENU:
            menuSystem->Render(renderer.get());
            break;
            
        case GameState::PLAYING:
        case GameState::PAUSED:
            if (map) {
                map->Render(renderer.get(), humanPlayerId);
            }
            
            if (hud) {
                hud->Render(renderer.get(), resourceManager.get(), 
                           selectionSystem.get());
            }
            
            if (CurrentGameState::CGS == GameState::PAUSED) {
                menuSystem->Render(renderer.get());
            }
            break;
            
        case GameState::WIN_SCREEN:
        case GameState::LOSE_SCREEN:
            ShowWinScreen();
            break;
            
        default:
            break;
    }
}

void Game::CheckWinCondition() {
    // Implementation for checking win/lose conditions
    // Check if player has any units/buildings left
    // Check if all enemies are defeated
}

void Game::HandleStateTransitions() {
    // Handle state transitions
}

void Game::ShowWinScreen() {
    // Show win or lose screen
    // Implementation for rendering win/lose screen
}

void Game::Shutdown() {
    audioManager->Shutdown();
    
    if (window) {
        glfwDestroyWindow(window);
    }
    
    glfwTerminate();
}