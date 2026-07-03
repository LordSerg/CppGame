#include "MenuSystem.h"
#include "../Graphics/Renderer.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <filesystem>

MenuSystem::MenuSystem()
    : selectedIndex(0)
    , selectedMapSize(1000)
{
}

void MenuSystem::Update(float deltaTime) {
    // Handle input for menu navigation
}

void MenuSystem::Render(Renderer* renderer) {
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoTitleBar | 
                                    ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoMove |
                                    ImGuiWindowFlags_NoCollapse;
    
    ImGui::SetNextWindowPos(ImVec2(renderer->GetWidth() / 2 - 200, 
                                   renderer->GetHeight() / 2 - 150));
    ImGui::SetNextWindowSize(ImVec2(400, 300));
    
    ImGui::Begin("Menu", nullptr, window_flags);
    
    for (size_t i = 0; i < currentMenuItems.size(); i++) {
        const MenuItem& item = currentMenuItems[i];
        
        if (ImGui::Button(item.label.c_str(), ImVec2(380, 40))) {
            if (item.callback && item.enabled) {
                item.callback();
            }
        }
        
        ImGui::Spacing();
    }
    
    ImGui::End();
}

void MenuSystem::ShowMainMenu() {
    currentMenuItems.clear();
    CurrentGameState::CGS = GameState::MAIN_MENU;
    
    currentMenuItems.push_back(MenuItem("New Game", [this]() {
        CurrentGameState::CGS = GameState::NEW_GAME_MENU;
        ShowNewGameMenu();
    }));
    
    currentMenuItems.push_back(MenuItem("Load Game", [this]() {
        CurrentGameState::CGS = GameState::LOAD_GAME_MENU;
        ShowLoadGameMenu();
    }));
    
    currentMenuItems.push_back(MenuItem("Exit", [this]() {
        if (stateChangeCallback) {
            stateChangeCallback(GameState::EXIT);
        }
    }));
}

void MenuSystem::ShowNewGameMenu() {
    currentMenuItems.clear();
    
    currentMenuItems.push_back(MenuItem("Small Map (500x500)", [this]() {
        if (newGameCallback) {
            newGameCallback(500);
        }
    }));
    
    currentMenuItems.push_back(MenuItem("Medium Map (1000x1000)", [this]() {
        if (newGameCallback) {
            newGameCallback(1000);
        }
    }));
    
    currentMenuItems.push_back(MenuItem("Large Map (2000x2000)", [this]() {
        if (newGameCallback) {
            newGameCallback(2000);
        }
    }));
    
    currentMenuItems.push_back(MenuItem("Mammoth Map (4000x4000)", [this]() {
        if (newGameCallback) {
            newGameCallback(4000);
        }
    }));
    
    currentMenuItems.push_back(MenuItem("Back to Main Menu", [this]() {
        ShowMainMenu();
    }));
}

void MenuSystem::ShowLoadGameMenu() {
    currentMenuItems.clear();
    
    LoadSavedGamesList();
    
    for (const std::string& saveName : savedGames) {
        currentMenuItems.push_back(MenuItem(saveName, [this, saveName]() {
            if (loadGameCallback) {
                loadGameCallback(saveName);
            }
        }));
    }
    
    currentMenuItems.push_back(MenuItem("Back to Main Menu", [this]() {
        ShowMainMenu();
    }));
}

void MenuSystem::ShowPauseMenu() {
    currentMenuItems.clear();
    CurrentGameState::CGS = GameState::PAUSED;
    
    currentMenuItems.push_back(MenuItem("Continue Game", [this]() {
        if (stateChangeCallback) {
            stateChangeCallback(GameState::PLAYING);
        }
    }));
    
    currentMenuItems.push_back(MenuItem("Save Game", [this]() {
        if (saveGameCallback) {
            saveGameCallback("quicksave");
        }
    }));
    
    currentMenuItems.push_back(MenuItem("Load Game", [this]() {
        ShowLoadGameMenu();
    }));
    
    currentMenuItems.push_back(MenuItem("Back to Main Menu", [this]() {
        ShowMainMenu();
    }));
}

void MenuSystem::ShowWinScreen(int playerScore, 
                              const std::vector<std::pair<std::string, int>>& scores) {
    currentMenuItems.clear();
    
    // Display victory screen with scores
    currentMenuItems.push_back(MenuItem("Back to Main Menu", [this]() {
        ShowMainMenu();
    }));
}

void MenuSystem::ShowLoseScreen(int playerScore, 
                               const std::vector<std::pair<std::string, int>>& scores) {
    currentMenuItems.clear();
    
    // Display defeat screen with scores
    currentMenuItems.push_back(MenuItem("Back to Main Menu", [this]() {
        ShowMainMenu();
    }));
}

void MenuSystem::SetStateChangeCallback(std::function<void(GameState)> callback) {
    stateChangeCallback = callback;
}

void MenuSystem::SetNewGameCallback(std::function<void(int)> callback) {
    newGameCallback = callback;
}

void MenuSystem::SetLoadGameCallback(std::function<void(const std::string&)> callback) {
    loadGameCallback = callback;
}

void MenuSystem::SetSaveGameCallback(std::function<void(const std::string&)> callback) {
    saveGameCallback = callback;
}

void MenuSystem::LoadSavedGamesList() {
    savedGames.clear();
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator("saves")) {
            if (entry.is_regular_file() && entry.path().extension() == ".sav") {
                savedGames.push_back(entry.path().stem().string());
            }
        }
    } catch (...) {
        // Directory doesn't exist or error reading
    }
}

void MenuSystem::RenderMenuItem(Renderer* renderer, const MenuItem& item, 
                               int index, int y) {
    // Rendering handled by ImGui
}