#include "FogOfWar.h"

FogOfWar::FogOfWar(int mapWidth, int mapHeight, int maxPlayers)
    : width(mapWidth)
    , height(mapHeight)
    , numPlayers(maxPlayers)
{
    fogData.resize(numPlayers);
    for (int p = 0; p < numPlayers; p++) {
        fogData[p].resize(height);
        for (int y = 0; y < height; y++) {
            fogData[p][y].resize(width, FogState::UNEXPLORED);
        }
    }
}

void FogOfWar::UpdateVision(int playerId, int x, int y, int visionRange) {
    if (playerId < 0 || playerId >= numPlayers) return;
    
    RevealArea(playerId, x, y, visionRange);
}

void FogOfWar::ClearVision(int playerId) {
    if (playerId < 0 || playerId >= numPlayers) return;
    
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            if (fogData[playerId][y][x] == FogState::VISIBLE) {
                fogData[playerId][y][x] = FogState::EXPLORED;
            }
        }
    }
}

FogState FogOfWar::GetFogState(int x, int y, int playerId) const {
    if (playerId < 0 || playerId >= numPlayers) return FogState::UNEXPLORED;
    if (x < 0 || x >= width || y < 0 || y >= height) return FogState::UNEXPLORED;
    
    return fogData[playerId][y][x];
}

bool FogOfWar::IsExplored(int x, int y, int playerId) const {
    FogState state = GetFogState(x, y, playerId);
    return state == FogState::EXPLORED || state == FogState::VISIBLE;
}

bool FogOfWar::IsVisible(int x, int y, int playerId) const {
    return GetFogState(x, y, playerId) == FogState::VISIBLE;
}

void FogOfWar::RevealArea(int playerId, int centerX, int centerY, int radius) {
    if (playerId < 0 || playerId >= numPlayers) return;
    
    for (int y = centerY - radius; y <= centerY + radius; y++) {
        for (int x = centerX - radius; x <= centerX + radius; x++) {
            if (x < 0 || x >= width || y < 0 || y >= height) continue;
            
            int dx = x - centerX;
            int dy = y - centerY;
            if (dx * dx + dy * dy <= radius * radius) {
                SetFogState(x, y, playerId, FogState::VISIBLE);
            }
        }
    }
}

void FogOfWar::SetFogState(int x, int y, int playerId, FogState state) {
    if (playerId < 0 || playerId >= numPlayers) return;
    if (x < 0 || x >= width || y < 0 || y >= height) return;
    
    fogData[playerId][y][x] = state;
}