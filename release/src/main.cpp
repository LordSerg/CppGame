#include "Core/Game.h"
#include <iostream>
#include <exception>

int main() {
    try {
        Game game;
        
        if (!game.Initialize()) {
            std::cerr << "Failed to initialize game!" << std::endl;
            return -1;
        }
        
        game.Run();
        game.Shutdown();
        
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return -1;
    }
    catch (...) {
        std::cerr << "Unknown exception occurred!" << std::endl;
        return -1;
    }
}