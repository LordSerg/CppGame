#include "Game.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <iostream>
#include <thread>
#include <mutex>

Game::Game() 
    : window(nullptr)
    , isRunning(false)
    , gameSpeed(1.0f)
    , gameTime(0)
    , currentState(GameState::MAIN_MENU)
    , previousState(GameState::MAIN_MENU)
    , screenWidth(1920)
    , screenHeight(1080)
    , humanPlayerId(0)
{
}

Game::~Game() {
}

// Static resize callback
static void WindowResizeCallback(GLFWwindow* window, int width, int height) {
    Game* game = static_cast<Game*>(glfwGetWindowUserPointer(window));
    if (game) {
        game->OnWindowResize(width, height);
    }
}

void Game::OnWindowResize(int width, int height) {
    screenWidth = width;
    screenHeight = height;
    if (renderer) {
        renderer->SetViewport(0, 0, width, height);
        if (renderer->GetCamera()) {
            renderer->GetCamera()->SetScreenSize(width, height);
        }
    }
    glViewport(0, 0, width, height);
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
    
    window = glfwCreateWindow(screenWidth, screenHeight, "Yet still untitled", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync
    
    // Set window user pointer for callbacks
    glfwSetWindowUserPointer(window, this);
    glfwSetWindowSizeCallback(window, WindowResizeCallback);
    
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
    menuSystem = std::make_unique<MenuSystem>();
    saveSystem = std::make_unique<SaveSystem>();
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
    
    // Setup menu callbacks
    SetupMenuCallbacks();

    // Show main menu
    menuSystem->ShowMainMenu();
    
    // Start background music
    std::cout<<"\nThe code of music playing was here\n";
    //audioManager->PlayMusic("//media//oknelaksoms//New Volume//shit7//release//assets//audio//music//main_theme.ogg", true);
    audioManager->PlayMusic("assets/audio/music/game_music.mp3", true);
    
    isRunning = true;
    return true;
}

void Game::SetupMenuCallbacks() {
    // State change callback
    menuSystem->SetStateChangeCallback([this](GameState newState) {
        if (newState == GameState::EXIT) {
            isRunning = false;
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        } else {
            ChangeState(newState);
        }
    });
    
    // New game callback
    menuSystem->SetNewGameCallback([this](int mapSize) {
        StartNewGame(static_cast<MapSize>(mapSize));
    });
    
    // Load game callback
    menuSystem->SetLoadGameCallback([this](const std::string& saveName) {
        LoadGameFromFile(saveName);
    });
    
    // Save game callback
    menuSystem->SetSaveGameCallback([this](const std::string& saveName) {
        SaveGameToFile(saveName);
    });
}

void Game::StartNewGame(MapSize size) {
    // Initialize game systems
    map = std::make_unique<Map>(size);
    map->Initialize();
    
    resourceManager = std::make_unique<ResourceManager>();
    techTree = std::make_unique<TechTree>();
    selectionSystem = std::make_unique<SelectionSystem>();
    commandSystem = std::make_unique<CommandSystem>(map.get());
    combatSystem = std::make_unique<CombatSystem>(map.get());
    populationSystem = std::make_unique<PopulationSystem>(map.get(), resourceManager.get());
    
    hud = std::make_unique<HUD>();
    hud->SetPlayerId(humanPlayerId);
    
    // Initialize players
    players.clear();
    
    // Human player
    players.push_back({
        0, "Player", glm::vec3(1.0f, 0.0f, 0.0f), true, true, 0
    });
    
    // AI player
    players.push_back({
        1, "AI", glm::vec3(0.0f, 0.0f, 1.0f), false, true, 0
    });
    
    // Initialize resources for all players
    for (const auto& player : players) {
        resourceManager->InitializePlayer(player.id);
        techTree->InitializePlayer(player.id);
    }
    
    // Create AI controller for AI players
    aiControllers.clear();
    for (const auto& player : players) {
        if (!player.isHuman) {
            aiControllers.push_back(
                std::make_unique<AIController>(player.id, AIDifficulty::MEDIUM)
            );
        }
    }
    
    // Place starting units and buildings
    PlaceStartingUnits();
    
    // Initialize fog of war and camera position before first render
    if (map) {
        map->UpdateFogOfWar(humanPlayerId);
    }
    
    // Set camera to player's starting position
    if (renderer && renderer->GetCamera()) {
        Camera* camera = renderer->GetCamera();
        camera->SetPosition(Vector2(
            50.0f * 32.0f, 
            50.0f * 32.0f
        ));
        
        // Set camera bounds to the map dimensions in world units
        // Map spans from (0,0) to (width*32, height*32) in world coordinates
        Rect mapBounds(0, 0, map->GetWidth() * 32, map->GetHeight() * 32);
        camera->SetBounds(mapBounds);
    }
    
    // Reset input state to prevent stale button presses from menu interaction
    if (inputHandler) {
        inputHandler->ResetState();
    }
    
    // Change to playing state
    ChangeState(GameState::PLAYING);
}

void Game::PlaceStartingUnits() {
    // Player starting position
    Point2D playerStart(50, 50);
    Point2D aiStart(450, 450);
    
    // Player starting units
    for (int i = 0; i < 5; i++) {
        auto peasant = std::make_shared<Peasant>(
            map->GetNextEntityId(), 
            humanPlayerId
        );
        peasant->SetPosition(
            (playerStart.x + i * 2) * 32.0f, 
            playerStart.y * 32.0f
        );
        map->AddEntity(peasant);
        populationSystem->OnUnitCreated(humanPlayerId);
    }
    
    // Player starting building (Hut)
    auto playerHut = std::make_shared<Building>(
        map->GetNextEntityId(),
        humanPlayerId,
        BuildingType::HUT
    );
    playerHut->SetPosition(playerStart.x * 32.0f, (playerStart.y + 5) * 32.0f);
    playerHut->StartConstruction();
    playerHut->SetConstructionProgress(1.0f); // Instant complete
    map->AddEntity(playerHut);
    techTree->OnBuildingCompleted(humanPlayerId, BuildingType::HUT);
    
    // AI starting units
    for (int i = 0; i < 5; i++) {
        auto peasant = std::make_shared<Peasant>(
            map->GetNextEntityId(), 
            1); // AI player ID
        peasant->SetPosition(
            (aiStart.x + i * 2) * 32.0f, 
            aiStart.y * 32.0f
        );
        map->AddEntity(peasant);
        populationSystem->OnUnitCreated(1);
    }
    
    // AI starting building
    auto aiHut = std::make_shared<Building>(
        map->GetNextEntityId(),
        1,
        BuildingType::HUT
    );
    aiHut->SetPosition(aiStart.x * 32.0f, (aiStart.y + 5) * 32.0f);
    aiHut->StartConstruction();
    aiHut->SetConstructionProgress(1.0f);
    map->AddEntity(aiHut);
    techTree->OnBuildingCompleted(1, BuildingType::HUT);
}

void Game::SaveGameToFile(const std::string& saveName) {
    if (!saveSystem->SaveGame(saveName)) {
        std::cerr << "Failed to save game: " << saveName << std::endl;
        return;
    }
    
    float gameTime = glfwGetTime();
    saveSystem->SaveGameState(map.get(), resourceManager.get(), techTree.get(), gameTime);
    saveSystem->FinishSave();
    
    std::cout << "Game saved: " << saveName << std::endl;
}

void Game::LoadGameFromFile(const std::string& saveName) {
    if (!saveSystem->LoadGame(saveName)) {
        std::cerr << "Failed to load game: " << saveName << std::endl;
        return;
    }
    
    // Create new game systems if they don't exist
    if (!map) {
        map = std::make_unique<Map>(MapSize::MEDIUM);
        map->Initialize();
    }
    
    if (!resourceManager) {
        resourceManager = std::make_unique<ResourceManager>();
    }
    
    if (!techTree) {
        techTree = std::make_unique<TechTree>();
    }
    
    if (!selectionSystem) {
        selectionSystem = std::make_unique<SelectionSystem>();
    }
    
    if (!commandSystem) {
        commandSystem = std::make_unique<CommandSystem>(map.get());
    }
    
    if (!combatSystem) {
        combatSystem = std::make_unique<CombatSystem>(map.get());
    }
    
    if (!populationSystem) {
        populationSystem = std::make_unique<PopulationSystem>(map.get(), resourceManager.get());
    }
    
    if (!hud) {
        hud = std::make_unique<HUD>();
        hud->SetPlayerId(humanPlayerId);
    }
    
    float gameTime;
    saveSystem->LoadGameState(map.get(), resourceManager.get(), techTree.get(), gameTime);
    
    std::cout << "Game loaded: " << saveName << std::endl;
    
    ChangeState(GameState::PLAYING);
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
    // Reset mouse-over-UI flag each frame - UI systems will re-set it if needed
    inputHandler->SetMouseOverUI(false);
    inputHandler->Update();
    
    // Check for escape key to pause
    if (CurrentGameState::CGS == GameState::PLAYING && 
        inputHandler->IsKeyPressed(GLFW_KEY_ESCAPE)) {
        ChangeState(GameState::PAUSED);
        menuSystem->ShowPauseMenu();
    }

    // Camera controls
    if (CurrentGameState::CGS == GameState::PLAYING && renderer && renderer->GetCamera()) {
        Camera* camera = renderer->GetCamera();
        
        float cameraSpeed = 500.0f * gameSpeed;
        
        if (inputHandler->IsKeyDown(GLFW_KEY_W) || inputHandler->IsKeyDown(GLFW_KEY_UP)) {
            camera->Move(Vector2(0, cameraSpeed * 0.016f));
        }
        if (inputHandler->IsKeyDown(GLFW_KEY_S) || inputHandler->IsKeyDown(GLFW_KEY_DOWN)) {
            camera->Move(Vector2(0, -cameraSpeed * 0.016f));
        }
        if (inputHandler->IsKeyDown(GLFW_KEY_A) || inputHandler->IsKeyDown(GLFW_KEY_LEFT)) {
            camera->Move(Vector2(-cameraSpeed * 0.016f, 0));
        }
        if (inputHandler->IsKeyDown(GLFW_KEY_D) || inputHandler->IsKeyDown(GLFW_KEY_RIGHT)) {
            camera->Move(Vector2(cameraSpeed * 0.016f, 0));
        }
        
        // Zoom
        float scrollDelta = inputHandler->GetScrollDelta();
        if (scrollDelta != 0) {
            camera->Zoom(scrollDelta * 0.1f);
        }
    }

    // Mini-map click/drag handling (update camera while left button is held over minimap)
    if (CurrentGameState::CGS == GameState::PLAYING && inputHandler->IsMouseButtonDown(MouseButton::LEFT)) {
        if (hud) {
            Vector2 mousePos = inputHandler->GetMousePosition();
            if (hud->HandleMinimapClick(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y),
                                       renderer->GetCamera(), map.get())) {
                // Minimap was interacted with - prevent selection system from processing
                if (inputHandler->IsMouseButtonPressed(MouseButton::LEFT)) {
                    inputHandler->EndSelection();
                }
                inputHandler->SetMouseOverUI(true);
            }
        }
    }
    
    // When left mouse button is released over the minimap, prevent selection clearing
    if (CurrentGameState::CGS == GameState::PLAYING && inputHandler->IsMouseButtonReleased(MouseButton::LEFT)) {
        if (hud) {
            Vector2 mousePos = inputHandler->GetMousePosition();
            if (hud->IsOverMinimap(static_cast<int>(mousePos.x), static_cast<int>(mousePos.y))) {
                inputHandler->SetMouseOverUI(true);
                inputHandler->EndSelection();
            }
        }
    }
    
    // Unit selection and commands
    if (CurrentGameState::CGS == GameState::PLAYING && !inputHandler->IsMouseOverUI()) {
        HandleGameInput();
    }
    
    // Reset scroll delta after camera has read it
    inputHandler->ResetScrollDelta();
}

void Game::HandleGameInput() {
    if (!map || !selectionSystem || !commandSystem) return;
    
    // Left click - selection
    if (inputHandler->IsMouseButtonPressed(MouseButton::LEFT)) {
        // Start selection box
    }
    
    if (inputHandler->IsMouseButtonReleased(MouseButton::LEFT)) {
        Rect selectionRect = inputHandler->GetSelectionRect();
        
        if (selectionRect.width > 5 && selectionRect.height > 5) {
            // Box selection
            Camera* camera = renderer->GetCamera();
            
            Vector2 topLeft = camera->ScreenToWorld(
                Vector2(selectionRect.x, selectionRect.y));
            Vector2 bottomRight = camera->ScreenToWorld(
                Vector2(selectionRect.x + selectionRect.width, 
                       selectionRect.y + selectionRect.height));
            
            // Ensure bottomRight is actually the bottom-right (topLeft may have been swapped by flip)
            float minX = std::min(topLeft.x, bottomRight.x);
            float maxX = std::max(topLeft.x, bottomRight.x);
            float minY = std::min(topLeft.y, bottomRight.y);
            float maxY = std::max(topLeft.y, bottomRight.y);
            
            // Convert world coordinates to tile indices using floor/ceil
            // to ensure the selection rect covers full tiles
            int tileMinX = (int)std::floor(minX / 32.0f);
            int tileMinY = (int)std::floor(minY / 32.0f);
            int tileMaxX = (int)std::ceil(maxX / 32.0f);
            int tileMaxY = (int)std::ceil(maxY / 32.0f);
            
            Rect worldRect(
                tileMinX, tileMinY,
                tileMaxX - tileMinX, tileMaxY - tileMinY
            );
            
            std::vector<Entity*> entities = map->GetEntitiesInRect(worldRect);
            selectionSystem->SelectEntitiesInRect(entities, worldRect, humanPlayerId);
        } else {
            // Single click selection
            Vector2 mousePos = inputHandler->GetMousePosition();
            Camera* camera = renderer->GetCamera();
            Vector2 worldPos = camera->ScreenToWorld(mousePos);
            
            Point2D gridPos(worldPos.x / 32, worldPos.y / 32);
            
            std::vector<Entity*> entities = map->GetEntitiesAt(gridPos.x, gridPos.y);
            
            if (!entities.empty()) {
                selectionSystem->ClearSelection();
                selectionSystem->SelectEntity(entities[0]);
            } else {
                selectionSystem->ClearSelection();
            }
        }
        
        // End selection after processing
        if (inputHandler) {
            inputHandler->EndSelection();
        }
    }
    
    // Right click - command
    if (inputHandler->IsMouseButtonPressed(MouseButton::RIGHT)) {
        auto selectedUnits = selectionSystem->GetSelectedUnits();
        
        if (!selectedUnits.empty()) {
            Vector2 mousePos = inputHandler->GetMousePosition();
            Camera* camera = renderer->GetCamera();
            Vector2 worldPos = camera->ScreenToWorld(mousePos);
            
            Point2D gridPos(worldPos.x / 32, worldPos.y / 32);
            
            // Check if clicking on an entity
            std::vector<Entity*> entities = map->GetEntitiesAt(gridPos.x, gridPos.y);
            
            if (!entities.empty() && entities[0]->GetOwnerId() != humanPlayerId) {
                // Attack enemy
                commandSystem->IssueAttack(selectedUnits, entities[0], false);
                audioManager->PlaySound("assets/audio/sfx/order_attack.ogg");
            } else {
                // Move to location
                commandSystem->IssueMove(selectedUnits, gridPos, false);
                audioManager->PlaySound("assets/audio/sfx/order_move.ogg");
            }
        }
    }
    
    // Hotkeys
    if (inputHandler->IsKeyPressed(GLFW_KEY_H)) {
        // Stop selected units
        auto selectedUnits = selectionSystem->GetSelectedUnits();
        if (!selectedUnits.empty()) {
            commandSystem->IssueStop(selectedUnits);
        }
    }
    
    // Game speed controls
    if (inputHandler->IsKeyPressed(GLFW_KEY_EQUAL) || 
        inputHandler->IsKeyPressed(GLFW_KEY_KP_ADD)) {
        gameSpeed = std::min(4.0f, gameSpeed * 1.5f);
    }
    if (inputHandler->IsKeyPressed(GLFW_KEY_MINUS) || 
        inputHandler->IsKeyPressed(GLFW_KEY_KP_SUBTRACT)) {
        gameSpeed = std::max(0.25f, gameSpeed / 1.5f);
    }
    if (inputHandler->IsKeyPressed(GLFW_KEY_BACKSPACE)) {
        gameSpeed = 1.0f;
    }
}

void Game::Update(float deltaTime) {
    switch (CurrentGameState::CGS) {
        case GameState::MAIN_MENU:
        case GameState::NEW_GAME_MENU:
        case GameState::LOAD_GAME_MENU:
            menuSystem->Update(deltaTime);
            break;
            
        case GameState::PLAYING:
            gameTime += deltaTime;
            
            if (map) {
                // Multithreaded entity updates (safe: each entity is independent)
                auto allEntities = map->GetAllEntitiesShared();
                int numThreads = std::thread::hardware_concurrency();
                if (numThreads < 2) numThreads = 2;
                
                if (allEntities.size() > 10) {
                    std::vector<std::thread> threads;
                    int chunkSize = allEntities.size() / numThreads + 1;
                    
                    for (int t = 0; t < numThreads; t++) {
                        int start = t * chunkSize;
                        int end = std::min((int)allEntities.size(), start + chunkSize);
                        if (start >= (int)allEntities.size()) break;
                        
                        threads.emplace_back([&allEntities, start, end, deltaTime]() {
                            for (int i = start; i < end; i++) {
                                allEntities[i]->Update(deltaTime);
                            }
                        });
                    }
                    
                    for (auto& t : threads) {
                        t.join();
                    }
                } else {
                    for (auto& entity : allEntities) {
                        entity->Update(deltaTime);
                    }
                }
                
                // Single-threaded: remove dead entities and update fog
                map->RemoveDeadEntities();
                map->UpdateFogOfWar(humanPlayerId);
            }
            
            // Single-threaded systems (all share mutable state)
            if (commandSystem) {
                commandSystem->Update(deltaTime);
            }
            
            if (combatSystem) {
                combatSystem->Update(deltaTime);
            }
            
            // Update AI (single-threaded, accesses commandSystem, map, etc.)
            for (auto& ai : aiControllers) {
                ai->Update(deltaTime, map.get(), resourceManager.get(), 
                          techTree.get(), commandSystem.get());
            }
            
            // Update audio listener position
            if (audioManager && renderer) {
                Vector2 cameraPos = renderer->GetCamera()->GetPosition();
                audioManager->Update(cameraPos);
            }
            
            CheckWinCondition();
            break;
            
        case GameState::PAUSED:
            // Don't update game logic
            menuSystem->Update(deltaTime);
            break;

        case GameState::WIN_SCREEN:
        case GameState::LOSE_SCREEN:
            menuSystem->Update(deltaTime);
            break;
            
        case GameState::EXIT:
            isRunning = false;
            break;
            
        default:
            break;
    }
    
    HandleStateTransitions();
}

void Game::Render() {
    renderer->Clear();
    
    // Start ImGui frame - this must be done once per frame before any ImGui calls
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    
    switch (CurrentGameState::CGS) {
        case GameState::MAIN_MENU:
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
                           selectionSystem.get(), map.get(), 
                           renderer->GetCamera());
            }

            // Draw selection box in screen space
            if (inputHandler->IsSelecting()) {
                Rect selectionRect = inputHandler->GetSelectionRect();
                renderer->DrawScreenRect(selectionRect, glm::vec3(0.0f, 1.0f, 0.0f));
            }
            
            if (CurrentGameState::CGS == GameState::PAUSED) {
                menuSystem->Render(renderer.get());
            }
            break;
            
        case GameState::WIN_SCREEN:
        case GameState::LOSE_SCREEN:
            ShowEndScreen();
            break;
            
        default:
            break;
    }
    
    // End ImGui frame and render
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Game::CheckWinCondition() {
    if (!map) return;
    
    std::vector<int> alivePlayers;
    
    for (const auto& player : players) {
        if (!player.isAlive) continue;
        
        // Check if player has any units or buildings
        bool hasUnits = false;
        bool hasBuildings = false;
        
        for (const auto& entity : map->GetAllEntities()) {
            if (entity->GetOwnerId() == player.id && entity->IsAlive()) {
                if (entity->GetType() == EntityType::UNIT) {
                    hasUnits = true;
                } else if (entity->GetType() == EntityType::BUILDING) {
                    Building* building = static_cast<Building*>(entity);
                    if (building->CountsForVictory()) {
                        hasBuildings = true;
                    }
                }
            }
        }
        
        if (hasUnits || hasBuildings) {
            alivePlayers.push_back(player.id);
        }
    }
    
    // Check for victory/defeat
    if (alivePlayers.size() == 1) {
        if (alivePlayers[0] == humanPlayerId) {
            ChangeState(GameState::WIN_SCREEN);
        } else {
            ChangeState(GameState::LOSE_SCREEN);
        }
    } else if (alivePlayers.empty()) {
        // Draw?
        ChangeState(GameState::LOSE_SCREEN);
    }
}

void Game::ShowEndScreen() {
    std::vector<std::pair<std::string, int>> scores;
    
    for (const auto& player : players) {
        scores.push_back({player.name, player.score});
    }
    
    if (currentState == GameState::WIN_SCREEN) {
        menuSystem->ShowWinScreen(players[humanPlayerId].score, scores);
    } else {
        menuSystem->ShowLoseScreen(players[humanPlayerId].score, scores);
    }
    
    menuSystem->Render(renderer.get());
}

void Game::ChangeState(GameState newState) {
    previousState = currentState;
    currentState = newState;
    CurrentGameState::CGS = newState;
}

void Game::HandleStateTransitions() {
    // Handle any necessary cleanup or setup when states change
    if (previousState != currentState) {
        switch (currentState) {
            case GameState::PLAYING:
                if (audioManager) {
                    audioManager->StopMusic();
                    audioManager->PlayMusic("assets/audio/music/game_music.mp3", true);
                }
                break;
                
            case GameState::MAIN_MENU:
                if (audioManager) {
                    audioManager->StopMusic();
                    audioManager->PlayMusic("assets/audio/music/main_theme.ogg", true);
                }
                break;
                
            default:
                break;
        }
        // Mark transition as handled
        previousState = currentState;
    }
}

void Game::Shutdown() {
    // ImGui cleanup
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    
    audioManager->Shutdown();
    
    if (window) {
        glfwDestroyWindow(window);
    }
    
    glfwTerminate();
}