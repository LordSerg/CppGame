#ifndef SELECTIONPANEL_H
#define SELECTIONPANEL_H

class Renderer;
class SelectionSystem;

class SelectionPanel {
public:
    SelectionPanel();
    
    void Render(Renderer* renderer, SelectionSystem* selectionSys);
    
private:
    void RenderUnitInfo(Renderer* renderer, class Unit* unit);
    void RenderBuildingInfo(Renderer* renderer, class Building* building);
    void RenderMultiSelection(Renderer* renderer, int count);
};

#endif // SELECTIONPANEL_H