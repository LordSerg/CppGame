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
    
private:
    int playerId;
    
    void RenderResourceBar(Renderer* renderer, ResourceManager* resourceMgr);
    void RenderMinimap(Renderer* renderer, Map* map, Camera* camera);
    void RenderSelectionPanel(Renderer* renderer, SelectionSystem* selectionSys);
    void RenderCommandButtons(Renderer* renderer, SelectionSystem* selectionSys);
};

#endif // HUD_H