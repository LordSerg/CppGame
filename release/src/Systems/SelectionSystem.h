#ifndef SELECTIONSYSTEM_H
#define SELECTIONSYSTEM_H

#include "../Entities/Entity.h"
#include "../Utils/Math.h"
#include <vector>
#include <memory>

class SelectionSystem {
public:
    SelectionSystem();
    
    void ClearSelection();
    void SelectEntity(Entity* entity);
    void DeselectEntity(Entity* entity);
    void SelectEntitiesInRect(const std::vector<Entity*>& entities, const Rect& rect, int playerId);
    
    bool IsSelected(Entity* entity) const;
    const std::vector<Entity*>& GetSelectedEntities() const { return selectedEntities; }
    int GetSelectionCount() const { return selectedEntities.size(); }
    
    bool HasSelection() const { return !selectedEntities.empty(); }
    bool IsSingleSelection() const { return selectedEntities.size() == 1; }
    bool IsMultiSelection() const { return selectedEntities.size() > 1; }
    
    Entity* GetFirstSelected() const;
    
    // Selection filtering
    std::vector<class Unit*> GetSelectedUnits() const;
    std::vector<class Building*> GetSelectedBuildings() const;
    
private:
    std::vector<Entity*> selectedEntities;
    const int maxSelectionCount = 25;
};

#endif // SELECTIONSYSTEM_H