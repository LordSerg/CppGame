#ifndef MENUSYSTEM_H
#define MENUSYSTEM_H

#include "../Core/GameState.h"
#include <functional>
#include <vector>
#include <string>

class Renderer;

struct MenuItem {
    std::string label;
    std::function<void()> callback;
    bool enabled;
    
    MenuItem(const std::string& l, std::function<void()> cb)
        : label(l), callback(cb), enabled(true) {}
};

class MenuSystem {
public:
    MenuSystem();
    
    void Update(float deltaTime);
    void Render(Renderer* renderer);
    
    void ShowMainMenu();
    void ShowNewGameMenu();
    void ShowLoadGameMenu();
    void ShowPauseMenu();
    void ShowWinScreen(int playerScore, const std::vector<std::pair<std::string, int>>& scores);
    void ShowLoseScreen(int playerScore, const std::vector<std::pair<std::string, int>>& scores);
    
    void SetStateChangeCallback(std::function<void(GameState)> callback);
    void SetNewGameCallback(std::function<void(int)> callback); // MapSize parameter
    void SetLoadGameCallback(std::function<void(const std::string&)> callback);
    void SetSaveGameCallback(std::function<void(const std::string&)> callback);
    
private:
    std::vector<MenuItem> currentMenuItems;
    int selectedIndex;
    
    std::function<void(GameState)> stateChangeCallback;
    std::function<void(int)> newGameCallback;
    std::function<void(const std::string&)> loadGameCallback;
    std::function<void(const std::string&)> saveGameCallback;
    
    std::vector<std::string> savedGames;
    int selectedMapSize;
    
    void LoadSavedGamesList();
    void RenderMenuItem(Renderer* renderer, const MenuItem& item, int index, int y);
};

#endif // MENUSYSTEM_H