#ifndef RESOURCEMANAGER_H
#define RESOURCEMANAGER_H

#include <unordered_map>
#include <string>
#include "../Entities/Unit.h"
#include "../Entities/Building.h"

struct PlayerResources {
    int wood;
    int metal;
    int food;
    
    int maxWood;
    int maxMetal;
    int maxFood;
    
    int currentPopulation;
    int maxPopulation;
    
    PlayerResources() 
        : wood(500), metal(0), food(100),
          maxWood(1000), maxMetal(1000), maxFood(1000),
          currentPopulation(0), maxPopulation(5) {}
};

struct ResourceCost {
    int wood;
    int metal;
    int food;
    int population;
    
    ResourceCost() : wood(0), metal(0), food(0), population(0) {}
    ResourceCost(int w, int m, int f, int p = 0) 
        : wood(w), metal(m), food(f), population(p) {}
};

class ResourceManager {
public:
    ResourceManager();
    
    // Resource management
    bool CanAfford(int playerId, const ResourceCost& cost) const;
    bool SpendResources(int playerId, const ResourceCost& cost);
    void AddResources(int playerId, int wood, int metal, int food);
    
    // Getters
    const PlayerResources& GetPlayerResources(int playerId) const;
    int GetWood(int playerId) const;
    int GetMetal(int playerId) const;
    int GetFood(int playerId) const;
    int GetCurrentPopulation(int playerId) const;
    int GetMaxPopulation(int playerId) const;
    
    // Population
    void AddPopulation(int playerId, int amount);
    void RemovePopulation(int playerId, int amount);
    void IncreaseMaxPopulation(int playerId, int amount);
    void DecreaseMaxPopulation(int playerId, int amount);
    
    // Storage capacity
    void IncreaseStorageCapacity(int playerId, int amount);
    void DecreaseStorageCapacity(int playerId, int amount);
    
    // Resource costs
    static ResourceCost GetUnitCost(UnitType unitType);
    static ResourceCost GetBuildingCost(BuildingType buildingType);
    static ResourceCost GetTechCost(const std::string& techName);
    static ResourceCost GetUpgradeCost(UnitType fromType, UnitType toType);
    
    // Serialization
    void Serialize(class SaveSystem* saveSystem);
    void Deserialize(class SaveSystem* saveSystem);
    
    void InitializePlayer(int playerId);
    
private:
    std::unordered_map<int, PlayerResources> playerResources;
    
};

#endif // RESOURCEMANAGER_H