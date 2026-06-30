#ifndef FOGOFWAR_H
#define FOGOFWAR_H

#include <vector>

enum class FogState {
    UNEXPLORED,
    EXPLORED,
    VISIBLE
};

class FogOfWar {
public:
    FogOfWar(int mapWidth, int mapHeight, int maxPlayers);
    
    void UpdateVision(int playerId, int x, int y, int visionRange);
    void ClearVision(int playerId);
    
    FogState GetFogState(int x, int y, int playerId) const;
    bool IsExplored(int x, int y, int playerId) const;
    bool IsVisible(int x, int y, int playerId) const;
    
    void RevealArea(int playerId, int centerX, int centerY, int radius);
    
private:
    int width;
    int height;
    int numPlayers;
    
    // [player][y][x]
    std::vector<std::vector<std::vector<FogState>>> fogData;
    
    void SetFogState(int x, int y, int playerId, FogState state);
};

#endif // FOGOFWAR_H