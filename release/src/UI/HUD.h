#ifndef HUD_H
#define HUD_H

#include <string>

class Renderer;
class ResourceManager;
class SelectionSystem;
class Map;
class Camera;
class Entity;

class HUD {
public:
    HUD();
    
    void Update(float deltaTime);
    void Render(Renderer* renderer, ResourceManager* resourceMgr, 
               SelectionSystem* selectionSys, Map* map, Camera* camera);
    
    void SetPlayerId(int id) { playerId = id; }
    
    bool IsMouseOverUI(int mouseX, int mouseY) const;
    
    // Handle click on minimap - moves camera to the clicked location
    // Returns true if the click was handled (inside minimap bounds)
    bool HandleMinimapClick(int mouseX, int mouseY, Camera* camera, Map* map) const;
    
private:
    int playerId;
    
    // Minimap layout constants
    static constexpr int MINIMAP_X = 10;
    static constexpr int MINIMAP_Y = 60;
    static constexpr int MINIMAP_SIZE = 200;
    static constexpr int TILE_SIZE = 32;
    
    void RenderResourceBar(Renderer* renderer, ResourceManager* resourceMgr);
    void RenderMinimap(Renderer* renderer, Map* map, Camera* camera);
    void RenderSelectionPanel(Renderer* renderer, SelectionSystem* selectionSys);
    void RenderCommandButtons(Renderer* renderer, SelectionSystem* selectionSys);
};

#endif // HUD_H