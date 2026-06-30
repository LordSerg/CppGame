#include "SelectionSystem.h"
#include "../Entities/Unit.h"
#include "../Entities/Building.h"
#include <algorithm>

SelectionSystem::SelectionSystem() {
}

void SelectionSystem::ClearSelection() {
    for (Entity* entity : selectedEntities) {
        entity->SetSelected(false);
    }
    selectedEntities.clear();
}

void SelectionSystem::SelectEntity(Entity* entity) {
    if (!entity) return;
    
    if (std::find(selectedEntities.begin(), selectedEntities.end(), entity) 
        == selectedEntities.end()) {
        selectedEntities.push_back(entity);
        entity->SetSelected(true);
    }
}

void SelectionSystem::DeselectEntity(Entity* entity) {
    if (!entity) return;
    
    auto it = std::find(selectedEntities.begin(), selectedEntities.end(), entity);
    if (it != selectedEntities.end()) {
        selectedEntities.erase(it);
        entity->SetSelected(false);
    }
}

void SelectionSystem::SelectEntitiesInRect(const std::vector<Entity*>& entities, 
                                          const Rect& rect, int playerId) {
    ClearSelection();
    
    std::vector<Entity*> unitsInRect;
    std::vector<Entity*> buildingsInRect;
    
    // Separate units and buildings
    for (Entity* entity : entities) {
        if (!entity->IsAlive()) continue;
        if (entity->GetOwnerId() != playerId) continue;
        
        if (rect.Intersects(entity->GetBounds())) {
            if (entity->GetType() == EntityType::UNIT) {
                unitsInRect.push_back(entity);
            } else if (entity->GetType() == EntityType::BUILDING) {
                buildingsInRect.push_back(entity);
            }
        }
    }
    
    // Priority: units first, then buildings
    if (!unitsInRect.empty()) {
        int count = 0;
        for (Entity* unit : unitsInRect) {
            if (count >= maxSelectionCount) break;
            SelectEntity(unit);
            count++;
        }
    } else if (!buildingsInRect.empty()) {
        // Select only one building
        SelectEntity(buildingsInRect[0]);
    }
}

bool SelectionSystem::IsSelected(Entity* entity) const {
    return std::find(selectedEntities.begin(), selectedEntities.end(), entity) 
           != selectedEntities.end();
}

Entity* SelectionSystem::GetFirstSelected() const {
    return selectedEntities.empty() ? nullptr : selectedEntities[0];
}

std::vector<Unit*> SelectionSystem::GetSelectedUnits() const {
    std::vector<Unit*> units;
    
    for (Entity* entity : selectedEntities) {
        if (entity->GetType() == EntityType::UNIT) {
            units.push_back(static_cast<Unit*>(entity));
        }
    }
    
    return units;
}

std::vector<Building*> SelectionSystem::GetSelectedBuildings() const {
    std::vector<Building*> buildings;
    
    for (Entity* entity : selectedEntities) {
        if (entity->GetType() == EntityType::BUILDING) {
            buildings.push_back(static_cast<Building*>(entity));
        }
    }
    
    return buildings;
}