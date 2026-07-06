#include "HUD.h"
#include "../Graphics/Renderer.h"
#include "../Systems/ResourceManager.h"
#include "../Systems/SelectionSystem.h"
#include "../Entities/Unit.h"
#include "../Entities/Building.h"
#include "../Graphics/Camera.h"
#include "../Map/Map.h"
#include "../Map/FogOfWar.h"
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
                SelectionSystem* selectionSys, Map* map, Camera* camera) {
    RenderResourceBar(renderer, resourceMgr);
    RenderMinimap(renderer, map, camera);
    RenderSelectionPanel(renderer, selectionSys);
    RenderCommandButtons(renderer, selectionSys);
}

bool HUD::IsMouseOverUI(int mouseX, int mouseY) const {
    // Check if mouse is over any UI element
    // Minimap area (10, 60, 210, 260)
    if (mouseX >= 10 && mouseX <= 210 && mouseY >= 60 && mouseY <= 260) return true;
    // Left panel area (1/4 of screen width) - approximate
    if (mouseX < 250) return true; // Left panel
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

void HUD::RenderMinimap(Renderer* renderer, Map* map, Camera* camera) {
    if (!map || !camera) return;
    
    int minimapSize = 200;
    int minimapX = 10;
    int minimapY = 60;
    
    ImGui::SetNextWindowPos(ImVec2(minimapX, minimapY));
    ImGui::SetNextWindowSize(ImVec2(minimapSize, minimapSize));
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                            ImGuiWindowFlags_NoResize |
                            ImGuiWindowFlags_NoMove;
    
    ImGui::Begin("Minimap", nullptr, flags);
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    
    int mapWidth = map->GetWidth();
    int mapHeight = map->GetHeight();
    
    // Scale factors: how many minimap pixels per map tile
    float scaleX = (float)minimapSize / mapWidth;
    float scaleY = (float)minimapSize / mapHeight;
    
    // Draw terrain based on fog of war
    // We iterate over the map in steps to keep performance reasonable
    int step = std::max(1, (mapWidth * mapHeight) / (minimapSize * minimapSize / 8));
    step = std::max(1, step);
    
    for (int y = 0; y < mapHeight; y += step) {
        for (int x = 0; x < mapWidth; x += step) {
            // Check fog of war
            if (!map->IsExplored(x, y, playerId)) {
                continue; // Unexplored - don't draw anything
            }
            
            // Determine color based on visibility and terrain
            ImU32 color;
            if (map->IsVisible(x, y, playerId)) {
                // Visible - show as green (terrain)
                color = IM_COL32(0, 180, 0, 255);
            } else {
                // Explored but not visible - show as darker green
                color = IM_COL32(0, 80, 0, 255);
            }
            
            float px = p.x + x * scaleX;
            float py = p.y + y * scaleY;
            float pw = std::max(1.0f, step * scaleX);
            float ph = std::max(1.0f, step * scaleY);
            
            draw_list->AddRectFilled(
                ImVec2(px, py),
                ImVec2(px + pw, py + ph),
                color
            );
        }
    }
    
    // Draw entities on minimap
    auto allEntities = map->GetAllEntities();
    for (Entity* entity : allEntities) {
        if (!entity->IsAlive()) continue;
        
        Point2D gridPos = entity->GetGridPosition();
        
        // Only show entities on visible tiles
        if (!map->IsVisible(gridPos.x, gridPos.y, playerId)) continue;
        
        float ex = p.x + gridPos.x * scaleX;
        float ey = p.y + gridPos.y * scaleY;
        
        // Entity size on minimap (at least 2x2 pixels)
        float ew = std::max(2.0f, entity->GetBounds().width * scaleX);
        float eh = std::max(2.0f, entity->GetBounds().height * scaleY);
        
        ImU32 entityColor;
        if (entity->GetOwnerId() == playerId) {
            // Ally - blue
            entityColor = IM_COL32(0, 100, 255, 255);
        } else {
            // Foe - red
            entityColor = IM_COL32(255, 0, 0, 255);
        }
        
        draw_list->AddRectFilled(
            ImVec2(ex, ey),
            ImVec2(ex + ew, ey + eh),
            entityColor
        );
    }
    
    // Draw viewport rectangle (white, unfilled)
    if (camera) {
        Vector2 camPos = camera->GetPosition();
        float zoom = camera->GetZoom();
        int screenW = renderer->GetWidth();
        int screenH = renderer->GetHeight();
        
        // Calculate visible world area in pixels
        float visibleWorldW = screenW / zoom;
        float visibleWorldH = screenH / zoom;
        
        // Convert to tile coordinates
        float viewLeft = (camPos.x - visibleWorldW / 2.0f) / 32.0f;
        float viewTop = (camPos.y - visibleWorldH / 2.0f) / 32.0f;
        float viewRight = (camPos.x + visibleWorldW / 2.0f) / 32.0f;
        float viewBottom = (camPos.y + visibleWorldH / 2.0f) / 32.0f;
        
        // Clamp to map bounds
        viewLeft = std::max(0.0f, viewLeft);
        viewTop = std::max(0.0f, viewTop);
        viewRight = std::min((float)mapWidth, viewRight);
        viewBottom = std::min((float)mapHeight, viewBottom);
        
        // Convert to minimap coordinates
        float vx = p.x + viewLeft * scaleX;
        float vy = p.y + viewTop * scaleY;
        float vw = (viewRight - viewLeft) * scaleX;
        float vh = (viewBottom - viewTop) * scaleY;
        
        // Draw unfilled white rectangle (only perimeter)
        draw_list->AddRect(
            ImVec2(vx, vy),
            ImVec2(vx + vw, vy + vh),
            IM_COL32(255, 255, 255, 255),
            0.0f,  // rounding
            0,     // flags
            1.5f   // thickness
        );
    }
    
    // Draw border around minimap
    draw_list->AddRect(
        p,
        ImVec2(p.x + minimapSize, p.y + minimapSize),
        IM_COL32(100, 100, 100, 255),
        0.0f,
        0,
        1.0f
    );
    
    ImGui::End();
}

void HUD::RenderSelectionPanel(Renderer* renderer, SelectionSystem* selectionSys) {
    int panelWidth = std::max(400, renderer->GetWidth() / 4);
    int panelHeight = renderer->GetHeight() - 270 - 200;
    
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
    int panelWidth = std::max(400, renderer->GetWidth() / 4);
    
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