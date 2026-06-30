#include "SelectionPanel.h"
#include "../Graphics/Renderer.h"
#include "../Systems/SelectionSystem.h"
#include "../Entities/Unit.h"
#include "../Entities/Building.h"

SelectionPanel::SelectionPanel() {
}

void SelectionPanel::Render(Renderer* renderer, SelectionSystem* selectionSys) {
    // Rendering is handled in HUD
}

void SelectionPanel::RenderUnitInfo(Renderer* renderer, Unit* unit) {
    // Unit info rendering
}

void SelectionPanel::RenderBuildingInfo(Renderer* renderer, Building* building) {
    // Building info rendering
}

void SelectionPanel::RenderMultiSelection(Renderer* renderer, int count) {
    // Multi-selection rendering
}