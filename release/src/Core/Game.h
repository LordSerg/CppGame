#ifndef GAME_H
#define GAME_H

#include "GameState.h"
#include "../Graphics/Renderer.h"
#include "../UI/MenuSystem.h"
#include "../UI/HUD.h"
#include "../Map/Map.h"
#include "../Input/InputHandler.h"
#include "../Audio/AudioManager.h"
#include "../Systems/ResourceManager.h"
#include "../Systems/TechTree.h"
#include "../Systems/SelectionSystem.h"
#include "../Systems/CommandSystem.h"
#include "../Systems/CombatSystem.h"
#include "../Systems/PopulationSystem.h"
#include "../AI/AIController.h"
#include "../Serialization/SaveSystem.h"
#include <GLFW/glfw3.h>
#include <memory>
#include <vector>

class Game {
public:
    Game();
    ~Game();

    bool Initialize();
    void Run();
    void Shutdown();

private:
    void ProcessInput();
    void Update(float deltaTime);
    void Render();
    
    void ChangeState(GameState newState);
    void HandleStateTransitions();
    
    // Game loop control
    GLFWwindow* window;
    bool isRunning;
    float gameSpeed;
    GameState currentState;
    GameState previousState;
    
    // Core systems
    std::unique_ptr<Renderer> renderer;
    std::unique_ptr<MenuSystem> menuSystem;
    std::unique_ptr<HUD> hud;
    std::unique_ptr<InputHandler> inputHandler;
    std::unique_ptr<AudioManager> audioManager;
    std::unique_ptr<SaveSystem> saveSystem;
    
    // Game systems
    std::unique_ptr<Map> map;
    std::unique_ptr<ResourceManager> resourceManager;
    std::unique_ptr<TechTree> techTree;
    std::unique_ptr<SelectionSystem> selectionSystem;
    std::unique_ptr<CommandSystem> commandSystem;
    std::unique_ptr<CombatSystem> combatSystem;
    std::unique_ptr<PopulationSystem> populationSystem;
    
    // AI
    std::vector<std::unique_ptr<AIController>> aiControllers;
    
    // Game settings
    int screenWidth;
    int screenHeight;
    
    // Player data
    struct PlayerData {
        int id;
        std::string name;
        glm::vec3 color;
        bool isHuman;
        bool isAlive;
        int score;
    };
    
    std::vector<PlayerData> players;
    int humanPlayerId;
    
    void CheckWinCondition();
    void ShowWinScreen();
    void ShowLoseScreen();
};

#endif // GAME_H