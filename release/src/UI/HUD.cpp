#include "HUD.h"
#include "../Graphics/Renderer.h"
#include "../Systems/ResourceManager.h"
#include "../Systems/SelectionSystem.h"
#include "../Entities/Unit.h"
#include "../Entities/Building.h"
#include <imgui.h>
#include <string>

HUD::HUD()
    : playerId(0)
{
}

void HUD::Update(float deltaTime) {
    // Update HUD elements
}

void HUD::Render(Renderer* renderer, ResourceManager* resourceMgr, 
                SelectionSystem* selectionSys) {
    RenderResourceBar(renderer, resourceMgr);
    RenderMinimap(renderer);
    RenderSelectionPanel(renderer, selectionSys);
    RenderCommandButtons(renderer, selectionSys);
}

bool HUD::IsMouseOverUI(int mouseX, int mouseY) const {
    // Check if mouse is over any UI element
    if (mouseX < renderer->GetWidth() / 4) return true; // Left panel
    if (mouseY < 50) return true; // Top resource bar
    return false;
}

void HUD::RenderResourceBar(Renderer* renderer, ResourceManager* resourceMgr) {
    const PlayerResources& res = resourceMgr->GetPlayerResources(playerId);
    
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(renderer->GetWidth(), 50));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove;
    
    ImGui::Begin("Resources", nullptr, flags);
    
    ImGui::Text("Wood: %d/%d", res.wood, res.maxWood);
    ImGui::SameLine();
    ImGui::Text("Metal: %d/%d", res.metal, res.maxMetal);
    ImGui::SameLine();
    ImGui::Text("Food: %d/%d", res.food, res.maxFood);
    ImGui::SameLine();
    ImGui::Text("Population: %d/%d", res.currentPopulation, res.maxPopulation);
    
    ImGui::End();
}

void HUD::RenderMinimap(Renderer* renderer) {
    int minimapSize = 200;
    int minimapX = 10;
    int minimapY = 60;
    
    ImGui::SetNextWindowPos(ImVec2(minimapX, minimapY));
    ImGui::SetNextWindowSize(ImVec2(minimapSize, minimapSize));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove;
    
    ImGui::Begin("Minimap", nullptr, flags);
    
    // Draw minimap representation
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    
    draw_list->AddRectFilled(p, 
                            ImVec2(p.x + minimapSize, p.y + minimapSize),
                            IM_COL32(50, 50, 50, 255));
    
    ImGui::End();
}

void HUD::RenderSelectionPanel(Renderer* renderer, SelectionSystem* selectionSys) {
    int panelWidth = renderer->GetWidth() / 4;
    int panelHeight = renderer->GetHeight() - 270;
    
    ImGui::SetNextWindowPos(ImVec2(10, 270));
    ImGui::SetNextWindowSize(ImVec2(panelWidth - 20, panelHeight));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove;
    
    ImGui::Begin("Selection", nullptr, flags);
    
    if (selectionSys->HasSelection()) {
        if (selectionSys->IsSingleSelection()) {
            Entity* selected = selectionSys->GetFirstSelected();
            
            if (selected->GetType() == EntityType::UNIT) {
                Unit* unit = static_cast<Unit*>(selected);
                
                ImGui::Text("Unit: %s", 
                    unit->GetUnitType() == UnitType::PEASANT ? "Peasant" : "Warrior");
                ImGui::Text("HP: %d/%d", unit->GetCurrentHealth(), unit->GetMaxHealth());
                ImGui::Text("Speed: %.1f", unit->GetSpeed());
                ImGui::Text("Attack: %d", unit->GetAttackDamage());
                ImGui::Text("Armor: %d", unit->GetArmor());
                
            } else if (selected->GetType() == EntityType::BUILDING) {
                Building* building = static_cast<Building*>(selected);
                
                ImGui::Text("Building");
                ImGui::Text("HP: %d/%d", 
                    building->GetCurrentHealth(), building->GetMaxHealth());
                
                if (building->IsUnderConstruction()) {
                    ImGui::ProgressBar(building->GetConstructionProgress());
                }
            }
        } else {
            ImGui::Text("Multiple units selected: %d", selectionSys->GetSelectionCount());
        }
    } else {
        ImGui::Text("No selection");
    }
    
    ImGui::End();
}

void HUD::RenderCommandButtons(Renderer* renderer, SelectionSystem* selectionSys) {
    int panelWidth = renderer->GetWidth() / 4;
    
    ImGui::SetNextWindowPos(ImVec2(10, renderer->GetHeight() - 200));
    ImGui::SetNextWindowSize(ImVec2(panelWidth - 20, 190));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove;
    
    ImGui::Begin("Commands", nullptr, flags);
    
    if (selectionSys->HasSelection()) {
        auto units = selectionSys->GetSelectedUnits();
        
        if (!units.empty()) {
            if (ImGui::Button("Move", ImVec2(80, 40))) {
                // Set move mode
            }
            ImGui::SameLine();
            if (ImGui::Button("Attack", ImVec2(80, 40))) {
                // Set attack mode
            }
            
            if (units[0]->GetUnitType() == UnitType::PEASANT) {
                if (ImGui::Button("Gather", ImVec2(80, 40))) {
                    // Set gather mode
                }
                ImGui::SameLine();
                if (ImGui::Button("Build", ImVec2(80, 40))) {
                    // Show build menu
                }
            }
        }
        
        auto buildings = selectionSys->GetSelectedBuildings();
        if (!buildings.empty()) {
            Building* building = buildings[0];
            
            if (building->CanProduceUnits()) {
                if (ImGui::Button("Train Peasant", ImVec2(120, 40))) {
                    building->ProduceUnit(UnitType::PEASANT);
                }
                
                if (building->GetBuildingType() == BuildingType::BARRACKS) {
                    if (ImGui::Button("Train Warrior", ImVec2(120, 40))) {
                        building->ProduceUnit(UnitType::WARRIOR);
                    }
                }
            }
            
            if (building->CanResearchTech()) {
                auto techs = building->GetAvailableTechs();
                for (const std::string& tech : techs) {
                    if (ImGui::Button(tech.c_str(), ImVec2(150, 30))) {
                        building->ResearchTech(tech);
                    }
                }
            }
        }
    }
    
    ImGui::End();
}