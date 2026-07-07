#include "Building.h"
#include "../Graphics/Renderer.h"
#include <algorithm>

Building::Building(int id, int ownerId, BuildingType buildingType)
    : Entity(id, EntityType::BUILDING, ownerId)
    , buildingType(buildingType)
    , state(BuildingState::BLUEPRINT)
    , constructionProgress(0)
    , constructionTimeTotal(10.0f)
    , currentProduction(UnitType(-1))
    , productionProgress(0)
    , productionTimeTotal(5.0f)
    , researchProgress(0)
    , researchTimeTotal(10.0f)
    , captureProgress(0)
    , captureTimeRequired(10.0f)
    , towerTarget(nullptr)
    , attackRange(5)
    , attackDamage(15)
    , attackSpeed(0.5f)
    , timeSinceLastShot(0)
    , populationProvided(0)
    , storageCapacity(0)
{
    // Set building properties based on type
    switch (buildingType) {
        case BuildingType::FARM:
            width = 3;
            height = 3;
            maxHealth = 200;
            populationProvided = 5;
            constructionTimeTotal = 8.0f;
            break;
            
        case BuildingType::SAWMILL:
            width = 3;
            height = 3;
            maxHealth = 250;
            constructionTimeTotal = 10.0f;
            break;
            
        case BuildingType::HUT:
            width = 2;
            height = 2;
            maxHealth = 150;
            constructionTimeTotal = 6.0f;
            break;
            
        case BuildingType::STORAGE:
            width = 3;
            height = 3;
            maxHealth = 300;
            storageCapacity = 500;
            constructionTimeTotal = 8.0f;
            break;
            
        case BuildingType::CRAFTSMAN_GUILD:
            width = 3;
            height = 3;
            maxHealth = 300;
            constructionTimeTotal = 15.0f;
            break;
            
        case BuildingType::FORGE:
            width = 3;
            height = 3;
            maxHealth = 300;
            constructionTimeTotal = 12.0f;
            break;
            
        case BuildingType::BARRACKS:
            width = 4;
            height = 4;
            maxHealth = 400;
            constructionTimeTotal = 15.0f;
            break;
            
        case BuildingType::ROAD:
            width = 1;
            height = 1;
            maxHealth = 50;
            constructionTimeTotal = 1.0f;
            break;
            
        case BuildingType::WALL:
            width = 1;
            height = 1;
            maxHealth = 200;
            constructionTimeTotal = 3.0f;
            break;
            
        case BuildingType::GATE:
            width = 1;
            height = 1;
            maxHealth = 250;
            constructionTimeTotal = 5.0f;
            break;
            
        case BuildingType::MINE:
            width = 3;
            height = 3;
            maxHealth = 300;
            constructionTimeTotal = 12.0f;
            break;
            
        case BuildingType::TOWER:
            width = 2;
            height = 2;
            maxHealth = 400;
            attackRange = 8;
            attackDamage = 20;
            constructionTimeTotal = 10.0f;
            break;
            
        case BuildingType::TRAINING_GROUND:
            width = 4;
            height = 4;
            maxHealth = 350;
            constructionTimeTotal = 12.0f;
            break;
    }
    
    currentHealth = maxHealth;
}

Building::~Building() {
}

void Building::Update(float deltaTime) {
    if (!IsAlive()) return;
    
    switch (state) {
        case BuildingState::UNDER_CONSTRUCTION:
            UpdateConstruction(deltaTime);
            break;
            
        case BuildingState::COMPLETED:
            if (IsProducingUnit()) {
                UpdateProduction(deltaTime);
            }
            
            if (IsResearchingTech()) {
                UpdateResearch(deltaTime);
            }
            
            if (buildingType == BuildingType::TOWER) {
                UpdateTowerCombat(deltaTime);
            }
            
            if (!capturingUnits.empty()) {
                UpdateCapture(deltaTime);
            }
            break;
            
        default:
            break;
    }
}

void Building::Render(Renderer* renderer) {
    Vector2 worldPos(position.x, position.y);
    
    // Load building texture
    Texture* buildingTex = renderer->LoadTexture("assets/textures/buildings/House1.png");
    
    if (buildingTex && state != BuildingState::BLUEPRINT) {
        // Render building with texture
        glm::vec3 color(1.0f, 0.9f, 0.8f); // Slight warm tint for player
        if (ownerId == 1) color = glm::vec3(0.8f, 0.8f, 1.0f); // Blue tint for AI
        
        if (state == BuildingState::UNDER_CONSTRUCTION) {
            color = color * 0.6f; // Darker when under construction
        }
        
        renderer->DrawTexturedRect(buildingTex, worldPos, 
                                   width * 32.0f, height * 32.0f, 
                                   color, 0.0f, 0.0f, 1.0f, 1.0f);
    } else {
        // Fallback to colored rect
        Vector2 screenPos = renderer->WorldToScreen(position);
        glm::vec3 color(0.8f, 0.4f, 0.2f);
        if (ownerId == 1) color = glm::vec3(0.3f, 0.3f, 0.8f);
        
        if (state == BuildingState::BLUEPRINT) {
            color = glm::vec3(0.5f, 0.5f, 0.5f);
        } else if (state == BuildingState::UNDER_CONSTRUCTION) {
            color = color * 0.7f;
        }
        
        renderer->DrawRect(
            Rect(screenPos.x, screenPos.y, width * 32, height * 32),
            color
        );
    }
    
    // Draw selection indicator (outline only)
    if (selected) {
        float bx = position.x - 2;
        float by = position.y - 2;
        float bw = width * 32 + 4;
        float bh = height * 32 + 4;
        glm::vec3 selColor(0.0f, 1.0f, 0.0f);
        renderer->DrawLine(Vector2(bx, by), Vector2(bx + bw, by), selColor, 2.0f);
        renderer->DrawLine(Vector2(bx + bw, by), Vector2(bx + bw, by + bh), selColor, 2.0f);
        renderer->DrawLine(Vector2(bx + bw, by + bh), Vector2(bx, by + bh), selColor, 2.0f);
        renderer->DrawLine(Vector2(bx, by + bh), Vector2(bx, by), selColor, 2.0f);
    }
    
    // Draw health bar
    float healthPercent = (float)currentHealth / maxHealth;
    renderer->DrawRect(
        Rect(position.x, position.y - 10, width * 32 * healthPercent, 5),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Draw construction progress
    if (state == BuildingState::UNDER_CONSTRUCTION) {
        renderer->DrawRect(
            Rect(position.x, position.y + height * 32 + 5, 
                 width * 32 * constructionProgress, 5),
            glm::vec3(1.0f, 1.0f, 0.0f)
        );
    }
}

void Building::StartConstruction() {
    state = BuildingState::UNDER_CONSTRUCTION;
    constructionProgress = 0;
}

void Building::AddConstructionProgress(float amount) {
    if (state != BuildingState::UNDER_CONSTRUCTION) return;
    
    constructionProgress += amount / constructionTimeTotal;
    constructionProgress = std::min(1.0f, constructionProgress);
    
    if (constructionProgress >= 1.0f) {
        OnConstructionComplete();
    }
}

void Building::SetConstructionProgress(float amount) {
    if (state != BuildingState::UNDER_CONSTRUCTION) return;
    
    constructionProgress = std::min(1.0f, std::max(0.0f, amount));
    
    if (constructionProgress >= 1.0f) {
        OnConstructionComplete();
    }
}

bool Building::IsConquerable() const {
    switch (buildingType) {
        case BuildingType::TOWER:
        case BuildingType::MINE:
        case BuildingType::WALL:
        case BuildingType::ROAD:
            return false;
        default:
            return true;
    }
}

bool Building::CountsForVictory() const {
    return buildingType != BuildingType::WALL && 
           buildingType != BuildingType::ROAD;
}

bool Building::CanProduceUnits() const {
    return buildingType == BuildingType::HUT || 
           buildingType == BuildingType::BARRACKS;
}

void Building::ProduceUnit(UnitType unitType) {
    if (!CanProduceUnits() || IsProducingUnit()) return;
    
    currentProduction = unitType;
    productionProgress = 0;
    
    switch (unitType) {
        case UnitType::PEASANT:
            productionTimeTotal = 5.0f;
            break;
        case UnitType::WARRIOR:
            productionTimeTotal = 8.0f;
            break;
        default:
            productionTimeTotal = 5.0f;
    }
}

bool Building::CanResearchTech() const {
    return buildingType == BuildingType::CRAFTSMAN_GUILD ||
           buildingType == BuildingType::FORGE ||
           buildingType == BuildingType::SAWMILL ||
           buildingType == BuildingType::MINE ||
           buildingType == BuildingType::TRAINING_GROUND;
}

std::vector<std::string> Building::GetAvailableTechs() const {
    std::vector<std::string> techs;
    
    // Based on building type, return available techs
    switch (buildingType) {
        case BuildingType::CRAFTSMAN_GUILD:
            techs.push_back("Urban planning");
            techs.push_back("Mining");
            techs.push_back("Military");
            break;
            
        case BuildingType::FORGE:
            techs.push_back("Sledgehammers");
            techs.push_back("Shields1");
            techs.push_back("Swords1");
            break;
            
        // ... other buildings
        
        default:
            break;
    }
    
    return techs;
}

void Building::ResearchTech(const std::string& techName) {
    if (!CanResearchTech() || IsResearchingTech()) return;
    
    currentResearch = techName;
    researchProgress = 0;
    researchTimeTotal = 15.0f; // Default research time
}

bool Building::CanUpgradeUnits() const {
    return buildingType == BuildingType::TRAINING_GROUND;
}

void Building::UpgradeUnit(Unit* unit) {
    if (!CanUpgradeUnits()) return;
    
    switch (unit->GetUnitType()) {
        case UnitType::PEASANT:
            unit->UpgradeToWarrior();
            break;
        case UnitType::WARRIOR:
            unit->UpgradeToVeteran();
            break;
        case UnitType::VETERAN:
            unit->UpgradeToPaladin();
            break;
        default:
            break;
    }
}

void Building::AddCapturingUnit(int unitId) {
    if (std::find(capturingUnits.begin(), capturingUnits.end(), unitId) 
        == capturingUnits.end()) {
        capturingUnits.push_back(unitId);
    }
}

void Building::RemoveCapturingUnit(int unitId) {
    capturingUnits.erase(
        std::remove(capturingUnits.begin(), capturingUnits.end(), unitId),
        capturingUnits.end()
    );
    
    if (capturingUnits.empty()) {
        captureProgress = 0;
    }
}

void Building::UpdateCapture(float deltaTime) {
    if (capturingUnits.size() >= 4) {
        captureProgress += deltaTime;
        
        if (captureProgress >= captureTimeRequired) {
            OnCaptureComplete();
        }
    } else {
        captureProgress = std::max(0.0f, captureProgress - deltaTime);
    }
}

void Building::UpdateConstruction(float deltaTime) {
    // Construction is updated by builder units
}

void Building::UpdateProduction(float deltaTime) {
    productionProgress += deltaTime;
    
    if (productionProgress >= productionTimeTotal) {
        OnProductionComplete();
    }
}

void Building::UpdateResearch(float deltaTime) {
    researchProgress += deltaTime;
    
    if (researchProgress >= researchTimeTotal) {
        OnResearchComplete();
    }
}

void Building::UpdateTowerCombat(float deltaTime) {
    if (towerTarget && (!towerTarget->IsAlive() || 
        position.Distance(towerTarget->GetPosition()) > attackRange * 32.0f)) {
        towerTarget = nullptr;
    }
    
    if (!towerTarget) {
        // Find new target in range
        // This would need access to Map
        return;
    }
    
    timeSinceLastShot += deltaTime;
    
    if (timeSinceLastShot >= 1.0f / attackSpeed) {
        towerTarget->TakeDamage(attackDamage);
        timeSinceLastShot = 0;
    }
}

void Building::OnConstructionComplete() {
    state = BuildingState::COMPLETED;
    constructionProgress = 1.0f;
}

void Building::OnProductionComplete() {
    // Spawn unit near building
    // This would need access to Map and game systems
    
    currentProduction = UnitType(-1);
    productionProgress = 0;
}

void Building::OnResearchComplete() {
    // Unlock tech for owner
    // This would need access to TechTree
    
    currentResearch.clear();
    researchProgress = 0;
}

void Building::OnCaptureComplete() {
    // Change owner to capturing units' owner
    // Would need to get owner from one of the capturing units
    
    capturingUnits.clear();
    captureProgress = 0;
}

void Building::Serialize(SaveSystem* saveSystem) {
    Entity::Serialize(saveSystem);
    // Serialize building-specific data
}

void Building::Deserialize(SaveSystem* saveSystem) {
    Entity::Deserialize(saveSystem);
    // Deserialize building-specific data
}