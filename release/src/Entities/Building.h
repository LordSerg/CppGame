#ifndef BUILDING_H
#define BUILDING_H

#include "Entity.h"
#include <string>
#include <vector>

enum class BuildingType {
    FARM,
    SAWMILL,
    HUT,
    STORAGE,
    CRAFTSMAN_GUILD,
    FORGE,
    BARRACKS,
    ROAD,
    WALL,
    GATE,
    MINE,
    TOWER,
    TRAINING_GROUND
};

enum class BuildingState {
    BLUEPRINT,
    UNDER_CONSTRUCTION,
    COMPLETED,
    DESTROYED
};

class Building : public Entity {
public:
    Building(int id, int ownerId, BuildingType buildingType);
    virtual ~Building();
    
    void Update(float deltaTime) override;
    void Render(class Renderer* renderer) override;
    
    // Construction
    void StartConstruction();
    void AddConstructionProgress(float amount);
    float GetConstructionProgress() const { return constructionProgress; }
    bool IsCompleted() const { return state == BuildingState::COMPLETED; }
    bool IsUnderConstruction() const { return state == BuildingState::UNDER_CONSTRUCTION; }
    bool IsBlueprint() const { return state == BuildingState::BLUEPRINT; }
    
    // Building info
    BuildingType GetBuildingType() const { return buildingType; }
    BuildingState GetState() const { return state; }
    bool IsConquerable() const;
    bool CountsForVictory() const;
    
    // Unit production
    bool CanProduceUnits() const;
    void ProduceUnit(UnitType unitType);
    bool IsProducingUnit() const { return currentProduction != UnitType(-1); }
    float GetProductionProgress() const { return productionProgress; }
    
    // Tech research
    bool CanResearchTech() const;
    std::vector<std::string> GetAvailableTechs() const;
    void ResearchTech(const std::string& techName);
    bool IsResearchingTech() const { return !currentResearch.empty(); }
    std::string GetCurrentResearch() const { return currentResearch; }
    float GetResearchProgress() const { return researchProgress; }
    
    // Unit upgrades
    bool CanUpgradeUnits() const;
    void UpgradeUnit(class Unit* unit);
    
    // Capturing
    void AddCapturingUnit(int unitId);
    void RemoveCapturingUnit(int unitId);
    int GetCapturingUnitsCount() const { return capturingUnits.size(); }
    float GetCaptureProgress() const { return captureProgress; }
    void UpdateCapture(float deltaTime);
    
    // Tower specific
    void SetAttackTarget(Entity* target) { towerTarget = target; }
    Entity* GetAttackTarget() const { return towerTarget; }
    int GetAttackRange() const { return attackRange; }
    int GetAttackDamage() const { return attackDamage; }
    
    // Population (for farms)
    int GetPopulationProvided() const { return populationProvided; }
    
    // Storage (for warehouses)
    int GetStorageCapacity() const { return storageCapacity; }
    
    // Serialization
    void Serialize(class SaveSystem* saveSystem) override;
    void Deserialize(class SaveSystem* saveSystem) override;
    
private:
    BuildingType buildingType;
    BuildingState state;
    
    // Construction
    float constructionProgress; // 0.0 to 1.0
    float constructionTimeTotal;
    
    // Unit production
    UnitType currentProduction;
    float productionProgress;
    float productionTimeTotal;
    
    // Tech research
    std::string currentResearch;
    float researchProgress;
    float researchTimeTotal;
    
    // Capturing
    std::vector<int> capturingUnits;
    float captureProgress;
    float captureTimeRequired;
    
    // Tower
    Entity* towerTarget;
    int attackRange;
    int attackDamage;
    float attackSpeed;
    float timeSinceLastShot;
    
    // Specific properties
    int populationProvided;
    int storageCapacity;
    
    void UpdateConstruction(float deltaTime);
    void UpdateProduction(float deltaTime);
    void UpdateResearch(float deltaTime);
    void UpdateTowerCombat(float deltaTime);
    
    void OnConstructionComplete();
    void OnProductionComplete();
    void OnResearchComplete();
    void OnCaptureComplete();
};

#endif // BUILDING_H